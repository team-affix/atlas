#ifndef DBUCT_SERIES_REDUCED_TREE_HPP
#define DBUCT_SERIES_REDUCED_TREE_HPP

#include <algorithm>
#include <list>
#include <set>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include "value_objects/srt_action.hpp"
#include "debug_assert.hpp"

template<typename NodeId>
struct dbuct_series_reduced_tree {
    using node_set_t  = std::unordered_set<NodeId>;
    using child_set_t = std::set<NodeId>;

    bool insert(NodeId node);
    bool link(NodeId parent, child_set_t children);

    const node_set_t& roots() const;
    const node_set_t& leaves() const;
    const child_set_t& children(NodeId parent) const;

    void push_frame();
    void pop_frame();

private:
    using children_map_t = std::unordered_map<NodeId, child_set_t>;
    using parents_map_t  = std::unordered_map<NodeId, NodeId>;

    struct frame {
        std::list<srt_action<NodeId>> actions;
    };

    void try_reduce(NodeId node);
    void reduce_nullary(NodeId node);
    void reduce_unary(NodeId parent);

    void log(srt_action<NodeId> action);
    void undo_action(const srt_action<NodeId>& action);

    node_set_t roots_;
    node_set_t leaves_;
    children_map_t children_;
    parents_map_t parents_;
    std::stack<frame> frame_stack_;
};

template<typename NodeId>
bool dbuct_series_reduced_tree<NodeId>::insert(NodeId node) {
    if (roots_.contains(node) || parents_.contains(node))
        return false;
    roots_.insert(node);
    leaves_.insert(node);
    log(srt_set_insert<NodeId>{srt_set_target::roots, node});
    log(srt_set_insert<NodeId>{srt_set_target::leaves, node});
    return true;
}

template<typename NodeId>
bool dbuct_series_reduced_tree<NodeId>::link(NodeId parent, child_set_t children) {
    if (!leaves_.contains(parent))
        return false;
    if (std::any_of(children.begin(), children.end(), [&](const NodeId& c) {
            return !roots_.contains(c);
        }))
        return false;

    children_.insert({parent, children});
    log(srt_children_insert<NodeId>{parent, children});

    for (const NodeId& c : children) {
        parents_.insert({c, parent});
        log(srt_parent_insert<NodeId>{c, parent});
    }

    leaves_.erase(parent);
    log(srt_set_erase<NodeId>{srt_set_target::leaves, parent});

    for (const NodeId& c : children) {
        roots_.erase(c);
        log(srt_set_erase<NodeId>{srt_set_target::roots, c});
    }

    try_reduce(parent);
    return true;
}

template<typename NodeId>
const typename dbuct_series_reduced_tree<NodeId>::node_set_t&
dbuct_series_reduced_tree<NodeId>::roots() const {
    return roots_;
}

template<typename NodeId>
const typename dbuct_series_reduced_tree<NodeId>::node_set_t&
dbuct_series_reduced_tree<NodeId>::leaves() const {
    return leaves_;
}

template<typename NodeId>
const typename dbuct_series_reduced_tree<NodeId>::child_set_t&
dbuct_series_reduced_tree<NodeId>::children(NodeId parent) const {
    return children_.at(parent);
}

template<typename NodeId>
void dbuct_series_reduced_tree<NodeId>::try_reduce(NodeId node) {
    auto it = children_.find(node);
    if (it == children_.end())
        return;
    const size_t child_count = it->second.size();
    if (child_count == 0)
        reduce_nullary(node);
    else if (child_count == 1)
        reduce_unary(node);
}

template<typename NodeId>
void dbuct_series_reduced_tree<NodeId>::reduce_nullary(NodeId node) {
    child_set_t captured = std::move(children_.at(node));
    children_.erase(node);
    log(srt_children_erase<NodeId>{node, std::move(captured)});

    auto grandparent_it = parents_.find(node);
    if (grandparent_it == parents_.end()) {
        roots_.erase(node);
        log(srt_set_erase<NodeId>{srt_set_target::roots, node});
    } else {
        const NodeId grandparent = grandparent_it->second;
        children_.at(grandparent).erase(node);
        log(srt_children_at_erase<NodeId>{grandparent, node});
        NodeId captured_parent = grandparent_it->second;
        parents_.erase(node);
        log(srt_parent_erase<NodeId>{node, captured_parent});
        try_reduce(grandparent);
    }
}

template<typename NodeId>
void dbuct_series_reduced_tree<NodeId>::reduce_unary(NodeId parent) {
    const NodeId child = *children_.at(parent).begin();

    child_set_t captured_children = std::move(children_.at(parent));
    children_.erase(parent);
    log(srt_children_erase<NodeId>{parent, std::move(captured_children)});

    auto grandparent_it = parents_.find(parent);
    if (grandparent_it == parents_.end()) {
        NodeId captured_parent = parents_.at(child);
        parents_.erase(child);
        log(srt_parent_erase<NodeId>{child, captured_parent});
        roots_.erase(parent);
        log(srt_set_erase<NodeId>{srt_set_target::roots, parent});
        roots_.insert(child);
        log(srt_set_insert<NodeId>{srt_set_target::roots, child});
    } else {
        const NodeId grandparent = grandparent_it->second;
        NodeId previous_parent = parents_.at(child);
        parents_.at(child) = grandparent;
        log(srt_parent_assign<NodeId>{child, previous_parent});
        children_.at(grandparent).erase(parent);
        log(srt_children_at_erase<NodeId>{grandparent, parent});
        children_.at(grandparent).insert(child);
        log(srt_children_at_insert<NodeId>{grandparent, child});
        parents_.erase(parent);
        log(srt_parent_erase<NodeId>{parent, grandparent});
    }
}

template<typename NodeId>
void dbuct_series_reduced_tree<NodeId>::push_frame() {
    frame_stack_.push(frame{});
}

template<typename NodeId>
void dbuct_series_reduced_tree<NodeId>::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions.rbegin(); it != current.actions.rend(); ++it)
        undo_action(*it);
}

template<typename NodeId>
void dbuct_series_reduced_tree<NodeId>::log(srt_action<NodeId> action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions.push_back(std::move(action));
}

template<typename NodeId>
void dbuct_series_reduced_tree<NodeId>::undo_action(const srt_action<NodeId>& action) {
    if (const auto* op = std::get_if<srt_set_insert<NodeId>>(&action)) {
        if (op->target == srt_set_target::roots)
            roots_.erase(op->node);
        else
            leaves_.erase(op->node);
    } else if (const auto* op = std::get_if<srt_set_erase<NodeId>>(&action)) {
        if (op->target == srt_set_target::roots)
            roots_.insert(op->node);
        else
            leaves_.insert(op->node);
    } else if (const auto* op = std::get_if<srt_children_insert<NodeId>>(&action)) {
        children_.erase(op->parent);
    } else if (const auto* op = std::get_if<srt_children_erase<NodeId>>(&action)) {
        children_.insert({op->parent, op->children});
    } else if (const auto* op = std::get_if<srt_parent_insert<NodeId>>(&action)) {
        parents_.erase(op->child);
    } else if (const auto* op = std::get_if<srt_parent_erase<NodeId>>(&action)) {
        parents_.insert({op->child, op->parent});
    } else if (const auto* op = std::get_if<srt_parent_assign<NodeId>>(&action)) {
        parents_.at(op->child) = op->previous;
    } else if (const auto* op = std::get_if<srt_children_at_insert<NodeId>>(&action)) {
        children_.at(op->parent).erase(op->child);
    } else if (const auto* op = std::get_if<srt_children_at_erase<NodeId>>(&action)) {
        children_.at(op->parent).insert(op->child);
    }
}

#endif
