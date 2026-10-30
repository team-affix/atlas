#ifndef PUD_LODESTAR_FOCUS_EXTENDER_HPP
#define PUD_LODESTAR_FOCUS_EXTENDER_HPP

#include <vector>
#include "value_objects/pud_global_node_id.hpp"

template<typename IContainsFocusNode, typename IInsertFocusNode>
struct pud_lodestar_focus_extender {
    pud_lodestar_focus_extender(IContainsFocusNode& contains_focus_node, IInsertFocusNode& insert_focus_node);
    void extend(pud_global_node_id parent, const std::vector<pud_global_node_id>& offspring);
private:
    IContainsFocusNode& contains_focus_node_;
    IInsertFocusNode& insert_focus_node_;
};

template<typename IContainsFocusNode, typename IInsertFocusNode>
pud_lodestar_focus_extender<IContainsFocusNode, IInsertFocusNode>::pud_lodestar_focus_extender(
    IContainsFocusNode& contains_focus_node, IInsertFocusNode& insert_focus_node)
    : contains_focus_node_(contains_focus_node)
    , insert_focus_node_(insert_focus_node) {}

template<typename IContainsFocusNode, typename IInsertFocusNode>
void pud_lodestar_focus_extender<IContainsFocusNode, IInsertFocusNode>::extend(
    pud_global_node_id parent, const std::vector<pud_global_node_id>& offspring) {
    if (!contains_focus_node_.contains(parent))
        return;
    for (const pud_global_node_id child : offspring)
        insert_focus_node_.insert(child);
}

#endif
