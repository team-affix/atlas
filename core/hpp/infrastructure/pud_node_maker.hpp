#ifndef PUD_NODE_MAKER_HPP
#define PUD_NODE_MAKER_HPP

#include <cstdint>
#include "value_objects/expr.hpp"
#include "value_objects/pud_axiom_group_id.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_node_delta.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/rule.hpp"

// Makes database nodes: stores them in the tree and records what follows from them. A root is an axiom
// of its group; a child is its parent's rule resolved on one of its goals, so it keeps every other goal
// of the parent, under the same ids, adds its own, and belongs to its parent's group.
template<
    typename IStoreAxiom,
    typename IStoreInference,
    typename IGetGoals,
    typename IStoreGoals,
    typename IGetLvc,
    typename IStoreLvc,
    typename IGetNodeAxiomGroup,
    typename IStoreNodeAxiomGroup,
    typename IAddAxiomGroupMember>
struct pud_node_maker {
    pud_node_maker(
        IStoreAxiom& store_axiom,
        IStoreInference& store_inference,
        IGetGoals& get_goals,
        IStoreGoals& store_goals,
        IGetLvc& get_lvc,
        IStoreLvc& store_lvc,
        IGetNodeAxiomGroup& get_node_axiom_group,
        IStoreNodeAxiomGroup& store_node_axiom_group,
        IAddAxiomGroupMember& add_axiom_group_member);
    pud_node_id make_root(const rule& axiom, pud_axiom_group_id group);
    pud_node_id make_child(const pud_node_delta& delta);
private:
    IStoreAxiom& store_axiom_;
    IStoreInference& store_inference_;
    IGetGoals& get_goals_;
    IStoreGoals& store_goals_;
    IGetLvc& get_lvc_;
    IStoreLvc& store_lvc_;
    IGetNodeAxiomGroup& get_node_axiom_group_;
    IStoreNodeAxiomGroup& store_node_axiom_group_;
    IAddAxiomGroupMember& add_axiom_group_member_;
};

#define PUD_NODE_MAKER_TEMPLATE                                                            \
    template<typename IStoreAxiom, typename IStoreInference, typename IGetGoals, typename IStoreGoals, \
             typename IGetLvc, typename IStoreLvc, typename IGetNodeAxiomGroup,             \
             typename IStoreNodeAxiomGroup, typename IAddAxiomGroupMember>
#define PUD_NODE_MAKER                                                                     \
    pud_node_maker<IStoreAxiom, IStoreInference, IGetGoals, IStoreGoals, IGetLvc, IStoreLvc, \
        IGetNodeAxiomGroup, IStoreNodeAxiomGroup, IAddAxiomGroupMember>

PUD_NODE_MAKER_TEMPLATE
PUD_NODE_MAKER::pud_node_maker(
    IStoreAxiom& store_axiom,
    IStoreInference& store_inference,
    IGetGoals& get_goals,
    IStoreGoals& store_goals,
    IGetLvc& get_lvc,
    IStoreLvc& store_lvc,
    IGetNodeAxiomGroup& get_node_axiom_group,
    IStoreNodeAxiomGroup& store_node_axiom_group,
    IAddAxiomGroupMember& add_axiom_group_member)
    : store_axiom_(store_axiom)
    , store_inference_(store_inference)
    , get_goals_(get_goals)
    , store_goals_(store_goals)
    , get_lvc_(get_lvc)
    , store_lvc_(store_lvc)
    , get_node_axiom_group_(get_node_axiom_group)
    , store_node_axiom_group_(store_node_axiom_group)
    , add_axiom_group_member_(add_axiom_group_member) {}

PUD_NODE_MAKER_TEMPLATE
pud_node_id PUD_NODE_MAKER::make_root(const rule& axiom, pud_axiom_group_id group) {
    const pud_node_id root = store_axiom_.store_axiom(axiom);
    pud_goal_list root_goals{.goals = {}, .next_id = 0};
    for (const expr* goal : axiom.body)
        root_goals = root_goals.adding(goal);
    store_goals_.store(root, root_goals);
    store_lvc_.store(root, axiom.var_count);
    store_node_axiom_group_.store(root, group);
    add_axiom_group_member_.add_member(group, root);
    return root;
}

PUD_NODE_MAKER_TEMPLATE
pud_node_id PUD_NODE_MAKER::make_child(const pud_node_delta& delta) {
    const pud_node_id child = store_inference_.store_inference(delta);
    pud_goal_list child_goals = get_goals_.get(delta.parent).closing(delta.call_site);
    for (const expr* goal : delta.body_goals)
        child_goals = child_goals.adding(goal);
    store_goals_.store(child, child_goals);
    store_lvc_.store(child, get_lvc_.get(delta.parent) + delta.var_count);
    store_node_axiom_group_.store(child, get_node_axiom_group_.get(delta.parent));
    return child;
}

#undef PUD_NODE_MAKER
#undef PUD_NODE_MAKER_TEMPLATE

#endif
