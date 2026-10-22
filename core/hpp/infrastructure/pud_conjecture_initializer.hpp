#ifndef PUD_CONJECTURE_INITIALIZER_HPP
#define PUD_CONJECTURE_INITIALIZER_HPP

#include <cstdint>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/pud_node_id.hpp"

template<typename IGetNextNodeID,
         typename IStoreHead,
         typename IStoreBodyGoals,
         typename IStoreVarCount,
         typename IMarkRoot,
         typename ISetLeaf>
struct pud_conjecture_initializer {
    pud_conjecture_initializer(IGetNextNodeID& get_next_node_id,
                               IStoreHead& store_head,
                               IStoreBodyGoals& store_body_goals,
                               IStoreVarCount& store_var_count,
                               IMarkRoot& mark_root,
                               ISetLeaf& set_leaf);
    pud_node_id initialize_conjecture(const expr* head, const std::vector<const expr*>& goals, uint32_t var_count);
private:
    IGetNextNodeID& get_next_node_id_;
    IStoreHead& store_head_;
    IStoreBodyGoals& store_body_goals_;
    IStoreVarCount& store_var_count_;
    IMarkRoot& mark_root_;
    ISetLeaf& set_leaf_;
};

template<typename IGNID, typename ISH, typename ISBG, typename ISVC, typename IMR, typename ISL>
pud_conjecture_initializer<IGNID, ISH, ISBG, ISVC, IMR, ISL>::pud_conjecture_initializer(
    IGNID& get_next_node_id,
    ISH& store_head,
    ISBG& store_body_goals,
    ISVC& store_var_count,
    IMR& mark_root,
    ISL& set_leaf)
    : get_next_node_id_(get_next_node_id)
    , store_head_(store_head)
    , store_body_goals_(store_body_goals)
    , store_var_count_(store_var_count)
    , mark_root_(mark_root)
    , set_leaf_(set_leaf)
{}

template<typename IGNID, typename ISH, typename ISBG, typename ISVC, typename IMR, typename ISL>
pud_node_id pud_conjecture_initializer<IGNID, ISH, ISBG, ISVC, IMR, ISL>::initialize_conjecture(
    const expr* head, const std::vector<const expr*>& goals, uint32_t var_count) {
    const pud_node_id id = get_next_node_id_.next();
    store_head_.store(id, head);
    store_body_goals_.store(id, goals);
    store_var_count_.store(id, var_count);
    mark_root_.mark_root(id);
    set_leaf_.set_leaf(id);
    return id;
}

#endif
