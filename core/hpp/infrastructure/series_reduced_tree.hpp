#ifndef SERIES_REDUCED_TREE_HPP
#define SERIES_REDUCED_TREE_HPP

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <set>

template<typename NodeId>
struct series_reduced_tree {
    bool insert(NodeId node);
    bool erase(NodeId node);
    bool link(NodeId parent, std::set<NodeId> children);
    bool unlink(NodeId child);
    void clear();

    const std::unordered_set<NodeId>& roots() const;
    const std::unordered_set<NodeId>& leaves() const;
    const std::set<NodeId>& children(NodeId parent) const;
    NodeId parent(NodeId child) const;

private:
    using children_it = typename std::unordered_map<NodeId, std::set<NodeId>>::iterator;

    void try_reduce(NodeId node);
    void reduce_nullary(NodeId node, children_it it);
    void reduce_unary(NodeId node, children_it it);

    std::unordered_set<NodeId> roots_;
    std::unordered_set<NodeId> leaves_;
    std::unordered_map<NodeId, std::set<NodeId>> children_;
    std::unordered_map<NodeId, NodeId> parents_;
};

template<typename NodeId>
bool series_reduced_tree<NodeId>::insert(NodeId node) {
    // Every in-forest node is either a root (roots_) or non-root (parents_).
    // Isolated nodes are in roots_; linked nodes are in parents_; branching roots are in roots_.
    if (roots_.contains(node) || parents_.contains(node))
        return false;
    roots_.insert(node);
    leaves_.insert(node);
    return true;
}

template<typename NodeId>
bool series_reduced_tree<NodeId>::erase(NodeId node) {
    // Only isolated nodes: in roots_ and leaves_ (no parent, no children row).
    // Linked leaves are in leaves_ but not roots_; branching nodes fail here.
    // To remove a linked leaf, unlink first. Former branching parents are absorbed by try_reduce.
    if (!roots_.contains(node) || !leaves_.contains(node))
        return false;
    roots_.erase(node);
    leaves_.erase(node);
    return true;
}

template<typename NodeId>
bool series_reduced_tree<NodeId>::link(NodeId parent, std::set<NodeId> children) {
    // Attach children to a leaf parent. Empty children removes the parent via try_reduce.
    // Parent must be leaf; each child must be an unlinked root.
    if (!leaves_.contains(parent))
        return false;

    if (std::any_of(children.begin(), children.end(), [&](const NodeId& c) {
            return !roots_.contains(c);
        }))
        return false;
    
    // wire them up
    children_.emplace(parent, children);
    for (const NodeId& c : children)
        parents_.emplace(c, parent);

    // the parent is no longer a leaf
    leaves_.erase(parent);

    // the children are no longer roots
    for (const NodeId& c : children)
        roots_.erase(c);
    
    try_reduce(parent);

    return true;
}

template<typename NodeId>
bool series_reduced_tree<NodeId>::unlink(NodeId child) {
    // the child must be linked
    auto child_parent_it = parents_.find(child);
    if (child_parent_it == parents_.end())
        return false;

    // get the parent
    const NodeId parent = child_parent_it->second;

    // unlink the child from the parent
    parents_.erase(child_parent_it);
    children_.at(parent).erase(child);

    // the child is now a root
    roots_.insert(child);

    // try to reduce the parent
    try_reduce(parent);

    return true;
}

template<typename NodeId>
void series_reduced_tree<NodeId>::clear() {
    roots_.clear();
    leaves_.clear();
    children_.clear();
    parents_.clear();
}

template<typename NodeId>
const std::unordered_set<NodeId>& series_reduced_tree<NodeId>::roots() const {
    return roots_;
}

template<typename NodeId>
const std::unordered_set<NodeId>& series_reduced_tree<NodeId>::leaves() const {
    return leaves_;
}

template<typename NodeId>
const std::set<NodeId>& series_reduced_tree<NodeId>::children(NodeId parent) const {
    return children_.at(parent);
}

template<typename NodeId>
NodeId series_reduced_tree<NodeId>::parent(NodeId child) const {
    return parents_.at(child);
}

template<typename NodeId>
void series_reduced_tree<NodeId>::try_reduce(NodeId node) {
    auto children_it = children_.find(node);
    if (children_it == children_.end())
        return;

    const size_t child_count = children_it->second.size();
    if (child_count == 0)
        reduce_nullary(node, children_it);
    else if (child_count == 1)
        reduce_unary(node, children_it);
}

template<typename NodeId>
void series_reduced_tree<NodeId>::reduce_nullary(NodeId node, children_it it) {
    children_.erase(it);
    leaves_.erase(node);

    auto grandparent_it = parents_.find(node);
    if (grandparent_it == parents_.end()) {
        roots_.erase(node);
    } else {
        const NodeId grandparent = grandparent_it->second;
        children_.at(grandparent).erase(node);
        parents_.erase(grandparent_it);
        try_reduce(grandparent);
    }
}

template<typename NodeId>
void series_reduced_tree<NodeId>::reduce_unary(NodeId parent, children_it it) {
    // replace the node with the child
    const NodeId child = *it->second.begin();
    
    // unlink the child from the parent
    children_.erase(it); // erase whole children entry
    
    auto grandparent_it = parents_.find(parent);
    
    if (grandparent_it == parents_.end()) {
        // the child is now a root
        parents_.erase(child);
        roots_.erase(parent);
        roots_.insert(child);
    } else {
        const auto grandparent = grandparent_it->second;
        
        // get children of the grandparent
        auto& grandparent_children = children_.at(grandparent);
        
        // link the child to the grandparent
        parents_.at(child) = grandparent;
        grandparent_children.erase(parent);
        grandparent_children.insert(child);
        
        // remove the absorbed parent from parents_
        parents_.erase(grandparent_it);
    }
}

#endif
