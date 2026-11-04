#ifndef EXPR_ISTREAM_EXTRACTOR_HPP
#define EXPR_ISTREAM_EXTRACTOR_HPP

#include <cstdint>
#include <istream>
#include <stdexcept>
#include <vector>
#include "value_objects/expr.hpp"

template<typename IMakeVar, typename IMakeFunctor, typename IExtractUint8, typename IExtractUint32>
struct expr_istream_extractor {
    expr_istream_extractor(
        IMakeVar& make_var,
        IMakeFunctor& make_functor,
        IExtractUint8& extract_uint8,
        IExtractUint32& extract_uint32);
    const expr* extract_expr(std::istream& in) const;
private:
    // A functor whose arguments are still being read.
    struct frame {
        uint32_t id;
        uint32_t argc;
        std::vector<const expr*> args;
    };

    IMakeVar& make_var_;
    IMakeFunctor& make_functor_;
    IExtractUint8& extract_uint8_;
    IExtractUint32& extract_uint32_;
};

#define EXPR_ISTREAM_EXTRACTOR_TEMPLATE \
    template<typename IMakeVar, typename IMakeFunctor, typename IExtractUint8, typename IExtractUint32>
#define EXPR_ISTREAM_EXTRACTOR \
    expr_istream_extractor<IMakeVar, IMakeFunctor, IExtractUint8, IExtractUint32>

EXPR_ISTREAM_EXTRACTOR_TEMPLATE
EXPR_ISTREAM_EXTRACTOR::expr_istream_extractor(
    IMakeVar& make_var,
    IMakeFunctor& make_functor,
    IExtractUint8& extract_uint8,
    IExtractUint32& extract_uint32)
    : make_var_(make_var)
    , make_functor_(make_functor)
    , extract_uint8_(extract_uint8)
    , extract_uint32_(extract_uint32) {}

EXPR_ISTREAM_EXTRACTOR_TEMPLATE
// Reads a pre-order term with an explicit stack instead of recursing, so a term nested arbitrarily deep cannot
// exhaust the call stack.
const expr* EXPR_ISTREAM_EXTRACTOR::extract_expr(std::istream& in) const {
    std::vector<frame> pending;
    while (true) {
        const expr* read = nullptr;
        const uint8_t tag = extract_uint8_.extract_uint8(in);
        if (tag == 0)
            read = make_var_.make_var(extract_uint32_.extract_uint32(in));
        else if (tag != 1)
            throw std::runtime_error("invalid expr tag");
        else {
            const uint32_t id = extract_uint32_.extract_uint32(in);
            const uint32_t argc = extract_uint32_.extract_uint32(in);
            if (argc != 0) {
                pending.push_back(frame{.id = id, .argc = argc, .args = {}});
                pending.back().args.reserve(argc);
                continue;
            }
            read = make_functor_.make_functor(id, {});
        }
        while (!pending.empty()) {
            frame& top = pending.back();
            top.args.push_back(read);
            if (top.args.size() < top.argc)
                break;
            read = make_functor_.make_functor(top.id, top.args);
            pending.pop_back();
        }
        if (pending.empty())
            return read;
    }
}

#undef EXPR_ISTREAM_EXTRACTOR
#undef EXPR_ISTREAM_EXTRACTOR_TEMPLATE

#endif
