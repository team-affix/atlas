#ifndef PUD_ANCESTOR_PATH_FINDER_HPP
#define PUD_ANCESTOR_PATH_FINDER_HPP

#include <algorithm>
#include <optional>
#include <vector>
#include "value_objects/pud_global_node_id.hpp"
#include "debug_assert.hpp"

// Finds stored paths by walking parent links up from a node: the path from its axiom root down to it, or the chain of
// nodes below an ancestor down to it.
template<typename IGetParent>
struct pud_ancestor_path_finder {
    pud_ancestor_path_finder(IGetParent& get_parent);
    std::vector<pud_global_node_id> find_path_from_root(pud_global_node_id node) const;
    std::vector<pud_global_node_id> find_path_below(pud_global_node_id ancestor, pud_global_node_id node) const;
private:
    IGetParent& get_parent_;
};

template<typename IGetParent>
pud_ancestor_path_finder<IGetParent>::pud_ancestor_path_finder(IGetParent& get_parent)
    : get_parent_(get_parent) {}

template<typename IGetParent>
std::vector<pud_global_node_id> pud_ancestor_path_finder<IGetParent>::find_path_from_root(
    pud_global_node_id node) const {
    std::vector<pud_global_node_id> path{node};
    for (std::optional<pud_global_node_id> parent = get_parent_.get_parent(node); parent.has_value();
         parent = get_parent_.get_parent(parent.value()))
        path.push_back(parent.value());
    std::reverse(path.begin(), path.end());
    return path;
}

template<typename IGetParent>
std::vector<pud_global_node_id> pud_ancestor_path_finder<IGetParent>::find_path_below(
    pud_global_node_id ancestor, pud_global_node_id node) const {
    std::vector<pud_global_node_id> chain;
    for (pud_global_node_id step = node; step != ancestor;) {
        chain.push_back(step);
        const std::optional<pud_global_node_id> parent = get_parent_.get_parent(step);
        DEBUG_ASSERT(parent.has_value());
        step = parent.value();
    }
    std::reverse(chain.begin(), chain.end());
    return chain;
}

#endif
