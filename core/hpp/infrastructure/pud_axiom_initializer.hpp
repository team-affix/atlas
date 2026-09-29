#ifndef PUD_AXIOM_INITIALIZER_HPP
#define PUD_AXIOM_INITIALIZER_HPP

#include <vector>
#include <unordered_map>
#include "value_objects/rule.hpp"
#include "value_objects/pud_node.hpp"
#include "value_objects/pud_specialization.hpp"
#include "value_objects/framed_expr.hpp"

template<typename IMakeNode, typename ILiftExpr>
struct pud_axiom_initializer {
    pud_axiom_initializer(IMakeNode& make_node, ILiftExpr& lift_expr);
    const pud_node* initialize_axiom(const rule& axiom);
private:
    IMakeNode& make_node_;
    ILiftExpr& lift_expr_;

    std::vector<const pud_node*> axioms_;
};

template<typename IMN, typename ILE>
pud_axiom_initializer<IMN, ILE>::pud_axiom_initializer(IMN& make_node, ILE& lift_expr)
    : make_node_(make_node)
    , lift_expr_(lift_expr)
{
}

template<typename IMN, typename ILE>
const pud_node* pud_axiom_initializer<IMN, ILE>::initialize_axiom(const rule& axiom) {

    // var 0 is the whole head. therefore if the axiom mentioned var 0,
    // it would be a self-reference. We thus need to bump all var indices by 1.

    const expr* shifted_head = lift_expr_.lift(axiom.head, 1);

    pud_specialization axiom_head_specialization {
        .var_idx = 0,
        .value = shifted_head,
    };

    std::vector<const expr*> shifted_body_goals;

    for (const expr* body_goal : axiom.body) {
        const expr* shifted_body_goal = lift_expr_.lift(body_goal, 1);
        shifted_body_goals.push_back(shifted_body_goal);
    }

    uint32_t var_count = 1 + axiom.var_count;

    return make_node_.make(
        {axiom_head_specialization},
        shifted_body_goals,
        var_count);
}

#endif
