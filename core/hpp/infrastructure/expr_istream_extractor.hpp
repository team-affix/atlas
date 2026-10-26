#ifndef EXPR_ISTREAM_EXTRACTOR_HPP
#define EXPR_ISTREAM_EXTRACTOR_HPP

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
const expr* EXPR_ISTREAM_EXTRACTOR::extract_expr(std::istream& in) const {
    const uint8_t tag = extract_uint8_.extract_uint8(in);
    if (tag == 0)
        return make_var_.make_var(extract_uint32_.extract_uint32(in));
    if (tag != 1)
        throw std::runtime_error("invalid expr tag");
    const uint32_t id = extract_uint32_.extract_uint32(in);
    const uint32_t argc = extract_uint32_.extract_uint32(in);
    std::vector<const expr*> args;
    args.reserve(argc);
    for (uint32_t arg = 0; arg < argc; ++arg)
        args.push_back(extract_expr(in));
    return make_functor_.make_functor(id, args);
}

#undef EXPR_ISTREAM_EXTRACTOR
#undef EXPR_ISTREAM_EXTRACTOR_TEMPLATE

#endif
