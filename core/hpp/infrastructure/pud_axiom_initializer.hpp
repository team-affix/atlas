#ifndef PUD_AXIOM_INITIALIZER_HPP
#define PUD_AXIOM_INITIALIZER_HPP

#include <vector>
#include <unordered_map>
#include "value_objects/rule.hpp"
#include "value_objects/pud_node.hpp"
#include "value_objects/pud_specialization.hpp"
#include "value_objects/framed_expr.hpp"

template<typename IMakeNode, typename INormalize>
struct pud_axiom_initializer {
    pud_axiom_initializer(IMakeNode& make_node, INormalize& normalize);
    const pud_node* initialize_axiom(const rule& axiom);
private:
    IMakeNode& make_node_;
    INormalize& normalize_;

    std::vector<const pud_node*> axioms_;
};

template<typename IMN, typename IN>
pud_axiom_initializer<IMN, IN>::pud_axiom_initializer(IMN& make_node, IN& normalize)
    : make_node_(make_node), normalize_(normalize)
{
}

template<typename IMN, typename IN>
const pud_node* pud_axiom_initializer<IMN, IN>::initialize_axiom(const rule& axiom) {

    // var 0 is the whole head. therefore if the axiom mentioned var 0,
    // it would be a self-reference. We thus need to bump all var indices by 1.

    std::unordered_map<uint32_t, uint32_t> translation;
    const expr* shifted_head = normalize_.normalize(
        framed_expr{axiom.head, 1}, 1, translation);
    
    pud_specialization axiom_head_specialization {
        .var_idx = 0,
        .value = shifted_head,
    };

    std::vector<const expr*> shifted_body_goals;

    for (const expr* body_goal : axiom.body) {
        const expr* shifted_body_goal = normalize_.normalize(
            framed_expr{body_goal, 1},
            1,
            translation);
        shifted_body_goals.push_back(shifted_body_goal);
    }

    uint32_t var_count = 1 + translation.size();

    return make_node_.make(
        {axiom_head_specialization},
        shifted_body_goals,
        var_count);
}

#endif
