#ifndef PUD_AXIOM_INITIALIZER_HPP
#define PUD_AXIOM_INITIALIZER_HPP

#include <vector>
#include "value_objects/rule.hpp"
#include "value_objects/pud_node_id.hpp"

template<typename IGetNextNodeID,
         typename IStoreHead,
         typename IStoreBodyGoals,
         typename IStoreVarCount,
         typename IMarkRoot,
         typename ISetLeaf,
         typename IMarkAxiom>
struct pud_axiom_initializer {
    pud_axiom_initializer(IGetNextNodeID& get_next_node_id,
                          IStoreHead& store_head,
                          IStoreBodyGoals& store_body_goals,
                          IStoreVarCount& store_var_count,
                          IMarkRoot& mark_root,
                          ISetLeaf& set_leaf,
                          IMarkAxiom& mark_axiom);
    pud_node_id initialize_axiom(const rule& axiom);
private:
    IGetNextNodeID& get_next_node_id_;
    IStoreHead& store_head_;
    IStoreBodyGoals& store_body_goals_;
    IStoreVarCount& store_var_count_;
    IMarkRoot& mark_root_;
    ISetLeaf& set_leaf_;
    IMarkAxiom& mark_axiom_;
};

template<typename IGNID, typename ISH, typename ISBG, typename ISVC, typename IMR, typename ISL, typename IMA>
pud_axiom_initializer<IGNID, ISH, ISBG, ISVC, IMR, ISL, IMA>::pud_axiom_initializer(
    IGNID& get_next_node_id,
    ISH& store_head,
    ISBG& store_body_goals,
    ISVC& store_var_count,
    IMR& mark_root,
    ISL& set_leaf,
    IMA& mark_axiom)
    : get_next_node_id_(get_next_node_id)
    , store_head_(store_head)
    , store_body_goals_(store_body_goals)
    , store_var_count_(store_var_count)
    , mark_root_(mark_root)
    , set_leaf_(set_leaf)
    , mark_axiom_(mark_axiom)
{}

template<typename IGNID, typename ISH, typename ISBG, typename ISVC, typename IMR, typename ISL, typename IMA>
pud_node_id pud_axiom_initializer<IGNID, ISH, ISBG, ISVC, IMR, ISL, IMA>::initialize_axiom(const rule& axiom) {
    const pud_node_id id = get_next_node_id_.next();
    store_head_.store(id, axiom.head);
    store_body_goals_.store(id, std::vector<const expr*>(axiom.body.begin(), axiom.body.end()));
    store_var_count_.store(id, axiom.var_count);
    mark_root_.mark_root(id);
    set_leaf_.set_leaf(id);
    mark_axiom_.mark_axiom(id);
    return id;
}

#endif
