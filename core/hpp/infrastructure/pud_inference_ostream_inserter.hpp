#ifndef PUD_INFERENCE_OSTREAM_INSERTER_HPP
#define PUD_INFERENCE_OSTREAM_INSERTER_HPP

#include <ostream>
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_substitution.hpp"

template<typename IInsertExpr, typename IInsertUint64, typename IInsertUint32>
struct pud_inference_ostream_inserter {
    pud_inference_ostream_inserter(
        IInsertExpr& insert_expr,
        IInsertUint64& insert_uint64,
        IInsertUint32& insert_uint32);
    void insert_inference(std::ostream& out, const pud_inference& value) const;
private:
    IInsertExpr& insert_expr_;
    IInsertUint64& insert_uint64_;
    IInsertUint32& insert_uint32_;
};

#define PUD_INFERENCE_OSTREAM_INSERTER_TEMPLATE template<typename IInsertExpr, typename IInsertUint64, typename IInsertUint32>
#define PUD_INFERENCE_OSTREAM_INSERTER pud_inference_ostream_inserter<IInsertExpr, IInsertUint64, IInsertUint32>

PUD_INFERENCE_OSTREAM_INSERTER_TEMPLATE
PUD_INFERENCE_OSTREAM_INSERTER::pud_inference_ostream_inserter(
    IInsertExpr& insert_expr,
    IInsertUint64& insert_uint64,
    IInsertUint32& insert_uint32)
    : insert_expr_(insert_expr)
    , insert_uint64_(insert_uint64)
    , insert_uint32_(insert_uint32) {}

PUD_INFERENCE_OSTREAM_INSERTER_TEMPLATE
void PUD_INFERENCE_OSTREAM_INSERTER::insert_inference(std::ostream& out, const pud_inference& value) const {
    insert_uint64_.insert_uint64(out, value.parent);
    insert_uint64_.insert_uint64(out, value.call_site);
    insert_uint64_.insert_uint64(out, value.substitutions.size());
    for (const pud_substitution& substitution : value.substitutions) {
        insert_uint32_.insert_uint32(out, substitution.var_idx);
        insert_expr_.insert_expr(out, substitution.value);
    }
    insert_uint64_.insert_uint64(out, value.body_goals.size());
    for (const expr* goal : value.body_goals)
        insert_expr_.insert_expr(out, goal);
    insert_uint32_.insert_uint32(out, value.var_count);
}

#undef PUD_INFERENCE_OSTREAM_INSERTER
#undef PUD_INFERENCE_OSTREAM_INSERTER_TEMPLATE

#endif
