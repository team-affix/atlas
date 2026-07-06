#ifndef DBUCT_SERIES_REDUCED_TREE_HPP
#define DBUCT_SERIES_REDUCED_TREE_HPP

#include <algorithm>
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include "infrastructure/backtrackable_map_assign.hpp"
#include "infrastructure/backtrackable_map_at_erase.hpp"
#include "infrastructure/backtrackable_map_at_insert.hpp"
#include "infrastructure/backtrackable_map_erase.hpp"
#include "infrastructure/backtrackable_map_insert.hpp"
#include "infrastructure/backtrackable_set_erase.hpp"
#include "infrastructure/backtrackable_set_insert.hpp"
#include "infrastructure/tracked.hpp"
#include "infrastructure/trail.hpp"

// Trail-journalled series-reduced tree: the delayed-backtracking counterpart of
// series_reduced_tree (which the restarting solvers keep and clear wholesale).
//
// The forest is held in four tracked containers on the shared trail. Every
// container mutation (including those inside the destructive unary/nullary
// reduction cascades) is a primitive backtrackable mutation, so a single
// link()+cascade logs a sequence of primitives whose LIFO replay on trail pop
// reconstructs the exact pre-link topology. No bespoke compound inverse is
// needed: the reduction that makes the tree "series-reduced" is undone step by
// step. All inner-set mutations capture the outer map and re-look-up the key on
// undo, so they survive a node being erased and re-inserted within the frame.
template<typename NodeId>
struct dbuct_series_reduced_tree {
    using node_set_t     = std::unordered_set<NodeId>;
    using child_set_t    = std::set<NodeId>;
    using children_map_t = std::unordered_map<NodeId, child_set_t>;
    using parents_map_t  = std::unordered_map<NodeId, NodeId>;

    explicit dbuct_series_reduced_tree(trail& t);

    bool insert(NodeId node);
    bool link(NodeId parent, child_set_t children);

    const node_set_t& roots() const;
    const node_set_t& leaves() const;
    const child_set_t& children(NodeId parent) const;

private:
    void try_reduce(NodeId node);
    void reduce_nullary(NodeId node);
    void reduce_unary(NodeId parent);

    tracked<node_set_t, trail>     roots_;
    tracked<node_set_t, trail>     leaves_;
    tracked<children_map_t, trail> children_;
    tracked<parents_map_t, trail>  parents_;
};

template<typename NodeId>
dbuct_series_reduced_tree<NodeId>::dbuct_series_reduced_tree(trail& t)
    : roots_(t, node_set_t{}), leaves_(t, node_set_t{}),
      children_(t, children_map_t{}), parents_(t, parents_map_t{}) {}

template<typename NodeId>
bool dbuct_series_reduced_tree<NodeId>::insert(NodeId node) {
    if (roots_.get().contains(node) || parents_.get().contains(node))
        return false;
    roots_.mutate(std::make_unique<backtrackable_set_insert<node_set_t>>(node));
    leaves_.mutate(std::make_unique<backtrackable_set_insert<node_set_t>>(node));
    return true;
}

template<typename NodeId>
bool dbuct_series_reduced_tree<NodeId>::link(NodeId parent, child_set_t children) {
    if (!leaves_.get().contains(parent))
        return false;
    if (std::any_of(children.begin(), children.end(), [&](const NodeId& c) {
            return !roots_.get().contains(c);
        }))
        return false;

    children_.mutate(std::make_unique<backtrackable_map_insert<children_map_t>>(parent, children));
    for (const NodeId& c : children)
        parents_.mutate(std::make_unique<backtrackable_map_insert<parents_map_t>>(c, parent));

    leaves_.mutate(std::make_unique<backtrackable_set_erase<node_set_t>>(parent));

    for (const NodeId& c : children)
        roots_.mutate(std::make_unique<backtrackable_set_erase<node_set_t>>(c));

    try_reduce(parent);

    return true;
}

template<typename NodeId>
const typename dbuct_series_reduced_tree<NodeId>::node_set_t&
dbuct_series_reduced_tree<NodeId>::roots() const { return roots_.get(); }

template<typename NodeId>
const typename dbuct_series_reduced_tree<NodeId>::node_set_t&
dbuct_series_reduced_tree<NodeId>::leaves() const { return leaves_.get(); }

template<typename NodeId>
const typename dbuct_series_reduced_tree<NodeId>::child_set_t&
dbuct_series_reduced_tree<NodeId>::children(NodeId parent) const { return children_.get().at(parent); }

template<typename NodeId>
void dbuct_series_reduced_tree<NodeId>::try_reduce(NodeId node) {
    auto it = children_.get().find(node);
    if (it == children_.get().end())
        return;

    const size_t child_count = it->second.size();
    if (child_count == 0)
        reduce_nullary(node);
    else if (child_count == 1)
        reduce_unary(node);
}

template<typename NodeId>
void dbuct_series_reduced_tree<NodeId>::reduce_nullary(NodeId node) {
    children_.mutate(std::make_unique<backtrackable_map_erase<children_map_t>>(node));

    auto grandparent_it = parents_.get().find(node);
    if (grandparent_it == parents_.get().end()) {
        roots_.mutate(std::make_unique<backtrackable_set_erase<node_set_t>>(node));
    } else {
        const NodeId grandparent = grandparent_it->second;
        children_.mutate(std::make_unique<backtrackable_map_at_erase<children_map_t>>(grandparent, node));
        parents_.mutate(std::make_unique<backtrackable_map_erase<parents_map_t>>(node));
        try_reduce(grandparent);
    }
}

template<typename NodeId>
void dbuct_series_reduced_tree<NodeId>::reduce_unary(NodeId parent) {
    const NodeId child = *children_.get().at(parent).begin();

    children_.mutate(std::make_unique<backtrackable_map_erase<children_map_t>>(parent));

    auto grandparent_it = parents_.get().find(parent);
    if (grandparent_it == parents_.get().end()) {
        parents_.mutate(std::make_unique<backtrackable_map_erase<parents_map_t>>(child));
        roots_.mutate(std::make_unique<backtrackable_set_erase<node_set_t>>(parent));
        roots_.mutate(std::make_unique<backtrackable_set_insert<node_set_t>>(child));
    } else {
        const NodeId grandparent = grandparent_it->second;
        parents_.mutate(std::make_unique<backtrackable_map_assign<parents_map_t>>(child, grandparent));
        children_.mutate(std::make_unique<backtrackable_map_at_erase<children_map_t>>(grandparent, parent));
        children_.mutate(std::make_unique<backtrackable_map_at_insert<children_map_t>>(grandparent, child));
        parents_.mutate(std::make_unique<backtrackable_map_erase<parents_map_t>>(parent));
    }
}

#endif
