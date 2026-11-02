#ifndef PUD_MODULE_ACTIVATOR_HPP
#define PUD_MODULE_ACTIVATOR_HPP

#include <deque>
#include <vector>
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_module_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "debug_assert.hpp"

template<
    typename IListAxioms,
    typename IIsNodeRefuted,
    typename IGetChildren,
    typename IActivateRoot,
    typename IRefuteLeaf,
    typename IMoveActivity>
struct pud_module_activator {
    pud_module_activator(
        IListAxioms& list_axioms,
        IIsNodeRefuted& is_node_refuted,
        IGetChildren& get_children,
        IActivateRoot& activate_root,
        IRefuteLeaf& refute_leaf,
        IMoveActivity& move_activity);
    pud_frontier_change activate_module(pud_module_id module);
private:
    IListAxioms& list_axioms_;
    IIsNodeRefuted& is_node_refuted_;
    IGetChildren& get_children_;
    IActivateRoot& activate_root_;
    IRefuteLeaf& refute_leaf_;
    IMoveActivity& move_activity_;
};

#define PUD_MODULE_ACTIVATOR_TEMPLATE                                                      \
    template<typename IListAxioms, typename IIsNodeRefuted, typename IGetChildren,      \
             typename IActivateRoot, typename IRefuteLeaf, typename IMoveActivity>
#define PUD_MODULE_ACTIVATOR                                                               \
    pud_module_activator<IListAxioms, IIsNodeRefuted, IGetChildren, IActivateRoot, IRefuteLeaf, IMoveActivity>

PUD_MODULE_ACTIVATOR_TEMPLATE
PUD_MODULE_ACTIVATOR::pud_module_activator(
    IListAxioms& list_axioms,
    IIsNodeRefuted& is_node_refuted,
    IGetChildren& get_children,
    IActivateRoot& activate_root,
    IRefuteLeaf& refute_leaf,
    IMoveActivity& move_activity)
    : list_axioms_(list_axioms)
    , is_node_refuted_(is_node_refuted)
    , get_children_(get_children)
    , activate_root_(activate_root)
    , refute_leaf_(refute_leaf)
    , move_activity_(move_activity) {}

PUD_MODULE_ACTIVATOR_TEMPLATE
pud_frontier_change PUD_MODULE_ACTIVATOR::activate_module(pud_module_id module) {
    std::deque<pud_global_node_id> descending;
    std::vector<pud_global_node_id> axioms_arrived_refuted;
    for (pud_global_node_id axiom : list_axioms_.list_axioms(module)) {
        if (is_node_refuted_.is_refuted(axiom))
            continue;
        descending.push_back(axiom);
        if (!activate_root_.activate_root(axiom))
            continue;
        axioms_arrived_refuted.push_back(axiom);
    }
    pud_frontier_change change{.offspring = {}, .refuted = {}};
    for (pud_global_node_id axiom : axioms_arrived_refuted) {
        const std::vector<pud_global_node_id> refuted = refute_leaf_.refute(axiom);
        change.refuted.insert(change.refuted.end(), refuted.begin(), refuted.end());
    }
    std::vector<pud_global_node_id> leaves;
    while (!descending.empty()) {
        const pud_global_node_id node = descending.front();
        descending.pop_front();
        if (is_node_refuted_.is_refuted(node))
            continue;
        const std::vector<pud_node_id>& stored_children = get_children_.get_children(node);
        if (stored_children.empty()) {
            leaves.push_back(node);
            continue;
        }
        std::vector<pud_global_node_id> children;
        for (pud_node_id stored_child : stored_children) {
            const pud_global_node_id child{.module = module, .node = stored_child};
            if (is_node_refuted_.is_refuted(child))
                continue;
            children.push_back(child);
        }
        DEBUG_ASSERT(!children.empty());
        const std::vector<pud_global_node_id> refuted = move_activity_.move(node, children);
        change.refuted.insert(change.refuted.end(), refuted.begin(), refuted.end());
        descending.insert(descending.end(), children.begin(), children.end());
    }
    for (pud_global_node_id leaf : leaves) {
        if (is_node_refuted_.is_refuted(leaf))
            continue;
        change.offspring.push_back(leaf);
    }
    return change;
}

#undef PUD_MODULE_ACTIVATOR
#undef PUD_MODULE_ACTIVATOR_TEMPLATE

#endif
