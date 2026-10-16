#ifndef PUD_NODE_PATH_FINDER_HPP
#define PUD_NODE_PATH_FINDER_HPP

#include <algorithm>
#include <vector>
#include "value_objects/pud_node_id.hpp"

template<typename ICheckNodeRoot, typename IGetNodeParent>
struct pud_node_path_finder {
    pud_node_path_finder(ICheckNodeRoot& check_node_root, IGetNodeParent& get_node_parent);
    std::vector<pud_node_id> find_path(pud_node_id node) const;
private:
    ICheckNodeRoot& check_node_root_;
    IGetNodeParent& get_node_parent_;
};

template<typename ICheckNodeRoot, typename IGetNodeParent>
pud_node_path_finder<ICheckNodeRoot, IGetNodeParent>::pud_node_path_finder(
    ICheckNodeRoot& check_node_root,
    IGetNodeParent& get_node_parent)
    : check_node_root_(check_node_root)
    , get_node_parent_(get_node_parent) {}

// Root first, the node itself last.
template<typename ICheckNodeRoot, typename IGetNodeParent>
std::vector<pud_node_id> pud_node_path_finder<ICheckNodeRoot, IGetNodeParent>::find_path(pud_node_id node) const {
    std::vector<pud_node_id> path{node};
    while (!check_node_root_.check_root(path.back()))
        path.push_back(get_node_parent_.get(path.back()));
    std::reverse(path.begin(), path.end());
    return path;
}

#endif
