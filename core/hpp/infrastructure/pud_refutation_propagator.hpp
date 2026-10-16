#ifndef PUD_REFUTATION_PROPAGATOR_HPP
#define PUD_REFUTATION_PROPAGATOR_HPP

#include <vector>
#include "value_objects/pud_node_id.hpp"
#include "debug_assert.hpp"

// A refuted node's whole subtree is refuted, so an already-refuted node is skipped together
// with everything below it. A parent whose children are all refuted is refuted too.
template<
    typename ICheckNodeRefuted,
    typename IMarkRefuted,
    typename IGetNodeChildren,
    typename IGetNodeParent,
    typename ICheckNodeRoot>
struct pud_refutation_propagator {
    pud_refutation_propagator(
        ICheckNodeRefuted& check_node_refuted,
        IMarkRefuted& mark_refuted,
        IGetNodeChildren& get_node_children,
        IGetNodeParent& get_node_parent,
        ICheckNodeRoot& check_node_root);
    void refute(pud_node_id node);
private:
    ICheckNodeRefuted& check_node_refuted_;
    IMarkRefuted& mark_refuted_;
    IGetNodeChildren& get_node_children_;
    IGetNodeParent& get_node_parent_;
    ICheckNodeRoot& check_node_root_;
};

#define PUD_REFUTATION_PROPAGATOR_TEMPLATE                                                 \
    template<typename ICheckNodeRefuted, typename IMarkRefuted, typename IGetNodeChildren, \
             typename IGetNodeParent, typename ICheckNodeRoot>
#define PUD_REFUTATION_PROPAGATOR                                                          \
    pud_refutation_propagator<ICheckNodeRefuted, IMarkRefuted, IGetNodeChildren, IGetNodeParent, \
        ICheckNodeRoot>

PUD_REFUTATION_PROPAGATOR_TEMPLATE
PUD_REFUTATION_PROPAGATOR::pud_refutation_propagator(
    ICheckNodeRefuted& check_node_refuted,
    IMarkRefuted& mark_refuted,
    IGetNodeChildren& get_node_children,
    IGetNodeParent& get_node_parent,
    ICheckNodeRoot& check_node_root)
    : check_node_refuted_(check_node_refuted)
    , mark_refuted_(mark_refuted)
    , get_node_children_(get_node_children)
    , get_node_parent_(get_node_parent)
    , check_node_root_(check_node_root) {}

PUD_REFUTATION_PROPAGATOR_TEMPLATE
void PUD_REFUTATION_PROPAGATOR::refute(pud_node_id node) {
    if (check_node_refuted_.check_refuted(node))
        return;

    std::vector<pud_node_id> pending{node};
    while (!pending.empty()) {
        const pud_node_id descendant = pending.back();
        pending.pop_back();
        if (check_node_refuted_.check_refuted(descendant))
            continue;
        mark_refuted_.mark_refuted(descendant);
        for (pud_node_id child : get_node_children_.get(descendant))
            pending.push_back(child);
    }

    pud_node_id climbed = node;
    while (!check_node_root_.check_root(climbed)) {
        const pud_node_id parent = get_node_parent_.get(climbed);
        DEBUG_ASSERT(!check_node_refuted_.check_refuted(parent));
        for (pud_node_id child : get_node_children_.get(parent)) {
            if (!check_node_refuted_.check_refuted(child))
                return;
        }
        mark_refuted_.mark_refuted(parent);
        climbed = parent;
    }
}

#undef PUD_REFUTATION_PROPAGATOR
#undef PUD_REFUTATION_PROPAGATOR_TEMPLATE

#endif
