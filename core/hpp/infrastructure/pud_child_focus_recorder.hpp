#ifndef PUD_CHILD_FOCUS_RECORDER_HPP
#define PUD_CHILD_FOCUS_RECORDER_HPP

#include "value_objects/pud_global_node_id.hpp"

// Fills the activate-child slot: a child of a focus node joins the focus, then the child is activated.
// Wrapping activation keeps the focus subtree inside the one activation pass from the axioms down to the
// leaves, on load and on every unfold. The focus axiom itself is inserted when the focus is chosen.
template<typename IContainsFocusNode, typename IInsertFocusNode, typename IActivateChild>
struct pud_child_focus_recorder {
    pud_child_focus_recorder(
        IContainsFocusNode& contains_focus_node, IInsertFocusNode& insert_focus_node, IActivateChild& activate_child);
    bool activate_child(pud_global_node_id parent, pud_global_node_id child);
private:
    IContainsFocusNode& contains_focus_node_;
    IInsertFocusNode& insert_focus_node_;
    IActivateChild& activate_child_;
};

#define PUD_CHILD_FOCUS_RECORDER_TEMPLATE \
    template<typename IContainsFocusNode, typename IInsertFocusNode, typename IActivateChild>
#define PUD_CHILD_FOCUS_RECORDER pud_child_focus_recorder<IContainsFocusNode, IInsertFocusNode, IActivateChild>

PUD_CHILD_FOCUS_RECORDER_TEMPLATE
PUD_CHILD_FOCUS_RECORDER::pud_child_focus_recorder(
    IContainsFocusNode& contains_focus_node, IInsertFocusNode& insert_focus_node, IActivateChild& activate_child)
    : contains_focus_node_(contains_focus_node)
    , insert_focus_node_(insert_focus_node)
    , activate_child_(activate_child) {}

PUD_CHILD_FOCUS_RECORDER_TEMPLATE
bool PUD_CHILD_FOCUS_RECORDER::activate_child(pud_global_node_id parent, pud_global_node_id child) {
    if (contains_focus_node_.contains(parent))
        insert_focus_node_.insert(child);
    return activate_child_.activate_child(parent, child);
}

#undef PUD_CHILD_FOCUS_RECORDER
#undef PUD_CHILD_FOCUS_RECORDER_TEMPLATE

#endif
