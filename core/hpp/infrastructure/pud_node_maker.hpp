#ifndef PUD_NODE_MAKER_HPP
#define PUD_NODE_MAKER_HPP

#include <cstdint>
#include <optional>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_child_delta.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_node_state.hpp"

// Makes database nodes. A root is a rule of its own; a child is its parent's rule resolved on one of
// its goals, so it keeps every other goal of the parent, under the same ids, and adds its own.
template<
    typename IGetNextNodeId,
    typename IStoreRootHead,
    typename IStoreParent,
    typename IStoreSubstitutions,
    typename IGetGoals,
    typename IStoreGoals,
    typename IGetLvc,
    typename IStoreLvc,
    typename IMarkNodeState>
struct pud_node_maker {
    pud_node_maker(
        IGetNextNodeId& get_next_node_id,
        IStoreRootHead& store_root_head,
        IStoreParent& store_parent,
        IStoreSubstitutions& store_substitutions,
        IGetGoals& get_goals,
        IStoreGoals& store_goals,
        IGetLvc& get_lvc,
        IStoreLvc& store_lvc,
        IMarkNodeState& mark_node_state);
    pud_node_id make_root(const expr* head, const std::vector<const expr*>& goals, uint32_t var_count);
    pud_node_id make_child(pud_node_id parent, pud_body_goal_id call_site, const pud_child_delta& delta);
private:
    IGetNextNodeId& get_next_node_id_;
    IStoreRootHead& store_root_head_;
    IStoreParent& store_parent_;
    IStoreSubstitutions& store_substitutions_;
    IGetGoals& get_goals_;
    IStoreGoals& store_goals_;
    IGetLvc& get_lvc_;
    IStoreLvc& store_lvc_;
    IMarkNodeState& mark_node_state_;
};

#define PUD_NODE_MAKER_TEMPLATE                                                            \
    template<typename IGetNextNodeId, typename IStoreRootHead, typename IStoreParent,       \
             typename IStoreSubstitutions, typename IGetGoals, typename IStoreGoals,        \
             typename IGetLvc, typename IStoreLvc, typename IMarkNodeState>
#define PUD_NODE_MAKER                                                                     \
    pud_node_maker<IGetNextNodeId, IStoreRootHead, IStoreParent, IStoreSubstitutions,      \
        IGetGoals, IStoreGoals, IGetLvc, IStoreLvc, IMarkNodeState>

PUD_NODE_MAKER_TEMPLATE
PUD_NODE_MAKER::pud_node_maker(
    IGetNextNodeId& get_next_node_id,
    IStoreRootHead& store_root_head,
    IStoreParent& store_parent,
    IStoreSubstitutions& store_substitutions,
    IGetGoals& get_goals,
    IStoreGoals& store_goals,
    IGetLvc& get_lvc,
    IStoreLvc& store_lvc,
    IMarkNodeState& mark_node_state)
    : get_next_node_id_(get_next_node_id)
    , store_root_head_(store_root_head)
    , store_parent_(store_parent)
    , store_substitutions_(store_substitutions)
    , get_goals_(get_goals)
    , store_goals_(store_goals)
    , get_lvc_(get_lvc)
    , store_lvc_(store_lvc)
    , mark_node_state_(mark_node_state) {}

PUD_NODE_MAKER_TEMPLATE
pud_node_id PUD_NODE_MAKER::make_root(const expr* head, const std::vector<const expr*>& goals, uint32_t var_count) {
    const pud_node_id root = get_next_node_id_.next();
    mark_node_state_.mark(root, pud_node_state::leaf);
    store_parent_.store(root, std::nullopt);
    store_root_head_.store(root, head);
    store_goals_.store(root, pud_goal_list{.goals = {}, .next_id = 0}.adding(goals));
    store_lvc_.store(root, var_count);
    return root;
}

PUD_NODE_MAKER_TEMPLATE
pud_node_id PUD_NODE_MAKER::make_child(pud_node_id parent, pud_body_goal_id call_site, const pud_child_delta& delta) {
    const pud_node_id child = get_next_node_id_.next();
    mark_node_state_.mark(child, pud_node_state::leaf);
    store_parent_.store(child, parent);
    store_substitutions_.store(child, delta.substitutions);
    store_goals_.store(child, get_goals_.get(parent).closing(call_site).adding(delta.body_goals));
    store_lvc_.store(child, get_lvc_.get(parent) + delta.var_count);
    return child;
}

#undef PUD_NODE_MAKER
#undef PUD_NODE_MAKER_TEMPLATE

#endif
