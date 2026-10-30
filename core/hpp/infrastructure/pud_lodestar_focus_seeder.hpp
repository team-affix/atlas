#ifndef PUD_LODESTAR_FOCUS_SEEDER_HPP
#define PUD_LODESTAR_FOCUS_SEEDER_HPP

#include <vector>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_module_id.hpp"
#include "value_objects/pud_node_id.hpp"

template<typename IGetChildren, typename IContainsFocusNode, typename IInsertFocusNode>
struct pud_lodestar_focus_seeder {
    pud_lodestar_focus_seeder(
        pud_global_node_id focus_axiom,
        IGetChildren& get_children,
        IContainsFocusNode& contains_focus_node,
        IInsertFocusNode& insert_focus_node);
    void seed(pud_module_id module);
private:
    pud_global_node_id focus_axiom_;
    IGetChildren& get_children_;
    IContainsFocusNode& contains_focus_node_;
    IInsertFocusNode& insert_focus_node_;
};

template<typename IGetChildren, typename IContainsFocusNode, typename IInsertFocusNode>
pud_lodestar_focus_seeder<IGetChildren, IContainsFocusNode, IInsertFocusNode>::pud_lodestar_focus_seeder(
    pud_global_node_id focus_axiom,
    IGetChildren& get_children,
    IContainsFocusNode& contains_focus_node,
    IInsertFocusNode& insert_focus_node)
    : focus_axiom_(focus_axiom)
    , get_children_(get_children)
    , contains_focus_node_(contains_focus_node)
    , insert_focus_node_(insert_focus_node) {}

template<typename IGetChildren, typename IContainsFocusNode, typename IInsertFocusNode>
void pud_lodestar_focus_seeder<IGetChildren, IContainsFocusNode, IInsertFocusNode>::seed(pud_module_id module) {
    if (module != focus_axiom_.module)
        return;
    std::vector<pud_global_node_id> pending{focus_axiom_};
    while (!pending.empty()) {
        const pud_global_node_id node = pending.back();
        pending.pop_back();
        for (const pud_node_id child_id : get_children_.get_children(node)) {
            const pud_global_node_id child{.module = node.module, .node = child_id};
            if (contains_focus_node_.contains(child))
                continue;
            insert_focus_node_.insert(child);
            pending.push_back(child);
        }
    }
}

#endif
