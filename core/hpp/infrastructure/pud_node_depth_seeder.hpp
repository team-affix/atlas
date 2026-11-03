#ifndef PUD_NODE_DEPTH_SEEDER_HPP
#define PUD_NODE_DEPTH_SEEDER_HPP

#include <cstddef>
#include <utility>
#include <vector>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_module_id.hpp"
#include "value_objects/pud_node_id.hpp"

// On load, records the depth of every node of a module: its axioms are at depth 0 and each stored
// inference is one deeper than its parent.
template<typename IListAxioms, typename IGetChildren, typename IInsertNodeDepth>
struct pud_node_depth_seeder {
    pud_node_depth_seeder(IListAxioms& list_axioms, IGetChildren& get_children, IInsertNodeDepth& insert_node_depth);
    void seed(pud_module_id module);
private:
    IListAxioms& list_axioms_;
    IGetChildren& get_children_;
    IInsertNodeDepth& insert_node_depth_;
};

#define PUD_NODE_DEPTH_SEEDER_TEMPLATE template<typename IListAxioms, typename IGetChildren, typename IInsertNodeDepth>
#define PUD_NODE_DEPTH_SEEDER          pud_node_depth_seeder<IListAxioms, IGetChildren, IInsertNodeDepth>

PUD_NODE_DEPTH_SEEDER_TEMPLATE
PUD_NODE_DEPTH_SEEDER::pud_node_depth_seeder(
    IListAxioms& list_axioms, IGetChildren& get_children, IInsertNodeDepth& insert_node_depth)
    : list_axioms_(list_axioms)
    , get_children_(get_children)
    , insert_node_depth_(insert_node_depth) {}

PUD_NODE_DEPTH_SEEDER_TEMPLATE
void PUD_NODE_DEPTH_SEEDER::seed(pud_module_id module) {
    std::vector<std::pair<pud_global_node_id, size_t>> pending;
    for (const pud_global_node_id axiom : list_axioms_.list_axioms(module))
        pending.emplace_back(axiom, 0);
    while (!pending.empty()) {
        const auto [node, depth] = pending.back();
        pending.pop_back();
        insert_node_depth_.insert(node, depth);
        for (const pud_node_id child : get_children_.get_children(node))
            pending.emplace_back(pud_global_node_id{.module = node.module, .node = child}, depth + 1);
    }
}

#undef PUD_NODE_DEPTH_SEEDER
#undef PUD_NODE_DEPTH_SEEDER_TEMPLATE

#endif
