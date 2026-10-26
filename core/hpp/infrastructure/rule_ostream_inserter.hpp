#ifndef RULE_OSTREAM_INSERTER_HPP
#define RULE_OSTREAM_INSERTER_HPP

#include <ostream>
#include "value_objects/expr.hpp"
#include "value_objects/rule.hpp"

template<typename IInsertExpr, typename IInsertUint64, typename IInsertUint32>
struct rule_ostream_inserter {
    rule_ostream_inserter(
        IInsertExpr& insert_expr,
        IInsertUint64& insert_uint64,
        IInsertUint32& insert_uint32);
    void insert_rule(std::ostream& out, const rule& value) const;
private:
    IInsertExpr& insert_expr_;
    IInsertUint64& insert_uint64_;
    IInsertUint32& insert_uint32_;
};

#define RULE_OSTREAM_INSERTER_TEMPLATE template<typename IInsertExpr, typename IInsertUint64, typename IInsertUint32>
#define RULE_OSTREAM_INSERTER rule_ostream_inserter<IInsertExpr, IInsertUint64, IInsertUint32>

RULE_OSTREAM_INSERTER_TEMPLATE
RULE_OSTREAM_INSERTER::rule_ostream_inserter(
    IInsertExpr& insert_expr,
    IInsertUint64& insert_uint64,
    IInsertUint32& insert_uint32)
    : insert_expr_(insert_expr)
    , insert_uint64_(insert_uint64)
    , insert_uint32_(insert_uint32) {}

RULE_OSTREAM_INSERTER_TEMPLATE
void RULE_OSTREAM_INSERTER::insert_rule(std::ostream& out, const rule& value) const {
    insert_expr_.insert_expr(out, value.head);
    insert_uint64_.insert_uint64(out, value.body.size());
    for (const expr* goal : value.body)
        insert_expr_.insert_expr(out, goal);
    insert_uint32_.insert_uint32(out, value.var_count);
}

#undef RULE_OSTREAM_INSERTER
#undef RULE_OSTREAM_INSERTER_TEMPLATE

#endif
