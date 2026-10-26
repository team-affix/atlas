#ifndef EXPR_OSTREAM_INSERTER_HPP
#define EXPR_OSTREAM_INSERTER_HPP

#include <ostream>
#include <variant>
#include "debug_assert.hpp"
#include "value_objects/expr.hpp"

template<typename IInsertUint8, typename IInsertUint32>
struct expr_ostream_inserter {
    expr_ostream_inserter(IInsertUint8& insert_uint8, IInsertUint32& insert_uint32);
    void insert_expr(std::ostream& out, const expr* term) const;
private:
    IInsertUint8& insert_uint8_;
    IInsertUint32& insert_uint32_;
};

#define EXPR_OSTREAM_INSERTER_TEMPLATE template<typename IInsertUint8, typename IInsertUint32>
#define EXPR_OSTREAM_INSERTER expr_ostream_inserter<IInsertUint8, IInsertUint32>

EXPR_OSTREAM_INSERTER_TEMPLATE
EXPR_OSTREAM_INSERTER::expr_ostream_inserter(IInsertUint8& insert_uint8, IInsertUint32& insert_uint32)
    : insert_uint8_(insert_uint8)
    , insert_uint32_(insert_uint32) {}

EXPR_OSTREAM_INSERTER_TEMPLATE
void EXPR_OSTREAM_INSERTER::insert_expr(std::ostream& out, const expr* term) const {
    if (const expr::var* variable = std::get_if<expr::var>(&term->content)) {
        insert_uint8_.insert_uint8(out, 0);
        insert_uint32_.insert_uint32(out, variable->index);
        return;
    }
    const expr::functor* functor = std::get_if<expr::functor>(&term->content);
    DEBUG_ASSERT(functor != nullptr);
    insert_uint8_.insert_uint8(out, 1);
    insert_uint32_.insert_uint32(out, functor->id);
    insert_uint32_.insert_uint32(out, static_cast<uint32_t>(functor->args.size()));
    for (const expr* arg : functor->args)
        insert_expr(out, arg);
}

#undef EXPR_OSTREAM_INSERTER
#undef EXPR_OSTREAM_INSERTER_TEMPLATE

#endif
