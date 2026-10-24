#ifndef PUD_MODULE_ACTIVATOR_HPP
#define PUD_MODULE_ACTIVATOR_HPP

#include <deque>
#include <vector>
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_module_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "debug_assert.hpp"

// Puts a registered module on the frontier by descending its stored tree. Every axiom not already
// refuted is activated. Then each node reached hands its place on the frontier to its children not
// already refuted, the way unfolding hands a leaf's place to its new children: every child is
// activated, then those refuted on arrival are refuted, then the node is deactivated. The descent
// stops at the leaves, which stay active. A node already refuted is skipped with everything below it.
template<
    typename IListAxioms,
    typename ICheckNodeRefuted,
    typename IGetChildren,
    typename IActivateRoot,
    typename IActivateChild,
    typename IRefuteLeaf,
    typename IDeactivateNode>
struct pud_module_activator {
    pud_module_activator(
        IListAxioms& list_axioms,
        ICheckNodeRefuted& check_node_refuted,
        IGetChildren& get_children,
        IActivateRoot& activate_root,
        IActivateChild& activate_child,
        IRefuteLeaf& refute_leaf,
        IDeactivateNode& deactivate_node);
    pud_frontier_change activate_module(pud_module_id module);
private:
    IListAxioms& list_axioms_;
    ICheckNodeRefuted& check_node_refuted_;
    IGetChildren& get_children_;
    IActivateRoot& activate_root_;
    IActivateChild& activate_child_;
    IRefuteLeaf& refute_leaf_;
    IDeactivateNode& deactivate_node_;
};

#define PUD_MODULE_ACTIVATOR_TEMPLATE                                                      \
    template<typename IListAxioms, typename ICheckNodeRefuted, typename IGetChildren,      \
             typename IActivateRoot, typename IActivateChild, typename IRefuteLeaf,        \
             typename IDeactivateNode>
#define PUD_MODULE_ACTIVATOR                                                               \
    pud_module_activator<IListAxioms, ICheckNodeRefuted, IGetChildren, IActivateRoot, IActivateChild, \
        IRefuteLeaf, IDeactivateNode>

PUD_MODULE_ACTIVATOR_TEMPLATE
PUD_MODULE_ACTIVATOR::pud_module_activator(
    IListAxioms& list_axioms,
    ICheckNodeRefuted& check_node_refuted,
    IGetChildren& get_children,
    IActivateRoot& activate_root,
    IActivateChild& activate_child,
    IRefuteLeaf& refute_leaf,
    IDeactivateNode& deactivate_node)
    : list_axioms_(list_axioms)
    , check_node_refuted_(check_node_refuted)
    , get_children_(get_children)
    , activate_root_(activate_root)
    , activate_child_(activate_child)
    , refute_leaf_(refute_leaf)
    , deactivate_node_(deactivate_node) {}

PUD_MODULE_ACTIVATOR_TEMPLATE
pud_frontier_change PUD_MODULE_ACTIVATOR::activate_module(pud_module_id module) {
    std::deque<pud_global_node_id> descending;
    std::vector<pud_global_node_id> axioms_arrived_refuted;
    for (pud_global_node_id axiom : list_axioms_.list_axioms(module)) {
        if (check_node_refuted_.check_refuted(axiom))
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
        if (check_node_refuted_.check_refuted(node))
            continue;
        const std::vector<pud_node_id>& stored_children = get_children_.get_children(node);
        if (stored_children.empty()) {
            leaves.push_back(node);
            continue;
        }
        std::vector<pud_global_node_id> children;
        for (pud_node_id stored_child : stored_children) {
            const pud_global_node_id child{.module = module, .node = stored_child};
            if (check_node_refuted_.check_refuted(child))
                continue;
            children.push_back(child);
        }
        DEBUG_ASSERT(!children.empty());
        std::vector<pud_global_node_id> children_arrived_refuted;
        for (pud_global_node_id child : children) {
            if (!activate_child_.activate_child(node, child))
                continue;
            children_arrived_refuted.push_back(child);
        }
        for (pud_global_node_id child : children_arrived_refuted) {
            const std::vector<pud_global_node_id> refuted = refute_leaf_.refute(child);
            change.refuted.insert(change.refuted.end(), refuted.begin(), refuted.end());
        }
        for (pud_global_node_id stranded : deactivate_node_.deactivate(node)) {
            const std::vector<pud_global_node_id> refuted = refute_leaf_.refute(stranded);
            change.refuted.insert(change.refuted.end(), refuted.begin(), refuted.end());
        }
        descending.insert(descending.end(), children.begin(), children.end());
    }
    for (pud_global_node_id leaf : leaves) {
        if (check_node_refuted_.check_refuted(leaf))
            continue;
        change.offspring.push_back(leaf);
    }
    return change;
}

#undef PUD_MODULE_ACTIVATOR
#undef PUD_MODULE_ACTIVATOR_TEMPLATE

#endif
