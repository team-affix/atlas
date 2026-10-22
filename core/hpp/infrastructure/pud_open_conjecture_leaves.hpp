#ifndef PUD_OPEN_CONJECTURE_LEAVES_HPP
#define PUD_OPEN_CONJECTURE_LEAVES_HPP

#include <set>
#include <unordered_set>
#include "value_objects/pud_node_id.hpp"

// Wraps opening and closing leaves to also track the open leaves of conjecture trees. A conjecture
// root is a root that is not an axiom; any other node belongs to its parent's tree, and every
// parent was open before it was expanded.
template<
    typename IMarkLeafOpen,
    typename IMarkLeafClosed,
    typename ICheckNodeRoot,
    typename ICheckAxiom,
    typename IGetNodeParent>
struct pud_open_conjecture_leaves {
    pud_open_conjecture_leaves(
        IMarkLeafOpen& mark_leaf_open,
        IMarkLeafClosed& mark_leaf_closed,
        ICheckNodeRoot& check_node_root,
        ICheckAxiom& check_axiom,
        IGetNodeParent& get_node_parent);
    void mark_leaf_open(pud_node_id leaf);
    void mark_leaf_closed(pud_node_id leaf);
    const std::set<pud_node_id>& get() const;
private:
    IMarkLeafOpen& mark_leaf_open_;
    IMarkLeafClosed& mark_leaf_closed_;
    ICheckNodeRoot& check_node_root_;
    ICheckAxiom& check_axiom_;
    IGetNodeParent& get_node_parent_;
    std::unordered_set<pud_node_id> conjecture_nodes_;
    std::set<pud_node_id> open_conjecture_leaves_;
};

#define PUD_OPEN_CONJECTURE_LEAVES_TEMPLATE                                                \
    template<typename IMarkLeafOpen, typename IMarkLeafClosed, typename ICheckNodeRoot,   \
             typename ICheckAxiom, typename IGetNodeParent>
#define PUD_OPEN_CONJECTURE_LEAVES                                                         \
    pud_open_conjecture_leaves<IMarkLeafOpen, IMarkLeafClosed, ICheckNodeRoot, ICheckAxiom, \
        IGetNodeParent>

PUD_OPEN_CONJECTURE_LEAVES_TEMPLATE
PUD_OPEN_CONJECTURE_LEAVES::pud_open_conjecture_leaves(
    IMarkLeafOpen& mark_leaf_open,
    IMarkLeafClosed& mark_leaf_closed,
    ICheckNodeRoot& check_node_root,
    ICheckAxiom& check_axiom,
    IGetNodeParent& get_node_parent)
    : mark_leaf_open_(mark_leaf_open)
    , mark_leaf_closed_(mark_leaf_closed)
    , check_node_root_(check_node_root)
    , check_axiom_(check_axiom)
    , get_node_parent_(get_node_parent)
    , conjecture_nodes_()
    , open_conjecture_leaves_() {}

PUD_OPEN_CONJECTURE_LEAVES_TEMPLATE
void PUD_OPEN_CONJECTURE_LEAVES::mark_leaf_open(pud_node_id leaf) {
    mark_leaf_open_.mark_leaf_open(leaf);
    const bool in_conjecture = check_node_root_.check_root(leaf)
        ? !check_axiom_.check_axiom(leaf)
        : conjecture_nodes_.contains(get_node_parent_.get(leaf));
    if (!in_conjecture)
        return;
    conjecture_nodes_.insert(leaf);
    open_conjecture_leaves_.insert(leaf);
}

PUD_OPEN_CONJECTURE_LEAVES_TEMPLATE
void PUD_OPEN_CONJECTURE_LEAVES::mark_leaf_closed(pud_node_id leaf) {
    open_conjecture_leaves_.erase(leaf);
    mark_leaf_closed_.mark_leaf_closed(leaf);
}

PUD_OPEN_CONJECTURE_LEAVES_TEMPLATE
const std::set<pud_node_id>& PUD_OPEN_CONJECTURE_LEAVES::get() const {
    return open_conjecture_leaves_;
}

#undef PUD_OPEN_CONJECTURE_LEAVES
#undef PUD_OPEN_CONJECTURE_LEAVES_TEMPLATE

#endif
