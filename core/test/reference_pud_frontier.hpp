#ifndef REFERENCE_PUD_FRONTIER_HPP
#define REFERENCE_PUD_FRONTIER_HPP

#include <deque>
#include <set>
#include <vector>
#include "infrastructure/pud_module.hpp"
#include "value_objects/pud_global_node_id.hpp"

// Walks of stored modules, independent of any runtime. The frontier is every leaf reached from a
// live axiom through live children. A stranded node is a live interior node reached that way with
// no live child; a consistent database has none.

inline std::set<pud_global_node_id> reference_pud_frontier(const std::deque<pud_module>& modules) {
    std::set<pud_global_node_id> frontier;
    for (const pud_module& module : modules) {
        std::vector<pud_node_id> pending;
        for (pud_node_id axiom = 0; axiom < module.count_axioms(); ++axiom) {
            if (!module.check_refuted(axiom))
                pending.push_back(axiom);
        }
        while (!pending.empty()) {
            const pud_node_id node = pending.back();
            pending.pop_back();
            const std::vector<pud_node_id>& children = module.get_children(node);
            if (children.empty()) {
                frontier.insert(pud_global_node_id{.module = module.get_module_id(), .node = node});
                continue;
            }
            for (pud_node_id child : children) {
                if (!module.check_refuted(child))
                    pending.push_back(child);
            }
        }
    }
    return frontier;
}

inline std::vector<pud_global_node_id> reference_pud_stranded_nodes(const std::deque<pud_module>& modules) {
    std::vector<pud_global_node_id> stranded;
    for (const pud_module& module : modules) {
        std::vector<pud_node_id> pending;
        for (pud_node_id axiom = 0; axiom < module.count_axioms(); ++axiom) {
            if (!module.check_refuted(axiom))
                pending.push_back(axiom);
        }
        while (!pending.empty()) {
            const pud_node_id node = pending.back();
            pending.pop_back();
            const std::vector<pud_node_id>& children = module.get_children(node);
            if (children.empty())
                continue;
            size_t live_children = 0;
            for (pud_node_id child : children) {
                if (module.check_refuted(child))
                    continue;
                pending.push_back(child);
                ++live_children;
            }
            if (live_children == 0)
                stranded.push_back(pud_global_node_id{.module = module.get_module_id(), .node = node});
        }
    }
    return stranded;
}

// Every node stored in the module, live or refuted: its axioms and everything below them.
inline size_t reference_pud_count_nodes(const pud_module& module) {
    size_t nodes = 0;
    std::vector<pud_node_id> pending;
    for (pud_node_id axiom = 0; axiom < module.count_axioms(); ++axiom)
        pending.push_back(axiom);
    while (!pending.empty()) {
        const pud_node_id node = pending.back();
        pending.pop_back();
        ++nodes;
        pending.insert(pending.end(), module.get_children(node).begin(), module.get_children(node).end());
    }
    return nodes;
}

#endif
