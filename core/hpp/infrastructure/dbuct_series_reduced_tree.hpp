#ifndef DBUCT_SERIES_REDUCED_TREE_HPP
#define DBUCT_SERIES_REDUCED_TREE_HPP

#include <algorithm>
#include <deque>
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
    dbuct_series_reduced_tree();

    bool insert(NodeId node);
    bool link(NodeId parent, std::set<NodeId> children);

    const std::unordered_set<NodeId>& roots() const;
    const std::unordered_set<NodeId>& leaves() const;
    const std::set<NodeId>& children(NodeId parent) const;

    void push_frame();
    void pop_frame();

private:
    using set_t       = std::unordered_set<NodeId>;
    using child_set_t = std::set<NodeId>;
    using map_t       = std::unordered_map<NodeId, child_set_t>;
    using parent_map_t = std::unordered_map<NodeId, NodeId>;

    struct frame {
        std::list<srt_action<NodeId>> actions_;
    };

    void try_reduce(NodeId node);
    void reduce_nullary(NodeId node);
    void reduce_unary(NodeId parent);

    void log(srt_action<NodeId> action);
    void undo_action(const srt_action<NodeId>& action);

    set_t roots_;
    set_t leaves_;
    map_t map_;
    parent_map_t parent_map_;
    std::stack<frame> frame_stack_;
};


template<typename NodeId>
dbuct_series_reduced_tree<NodeId>::dbuct_series_reduced_tree()
    : frame_stack_(std::deque<frame>{frame{}}) {}

template<typename NodeId>
bool dbuct_series_reduced_tree<NodeId>::insert(NodeId node) {
    if (roots_.contains(node) || parent_map_.contains(node))
        return false;
    roots_.insert(node);
    leaves_.insert(node);
    log(srt_set_insert<NodeId>{srt_set_target::roots, node});
    log(srt_set_insert<NodeId>{srt_set_target::leaves, node});
    return true;
}

template<typename NodeId>
bool dbuct_series_reduced_tree<NodeId>::link(NodeId parent, std::set<NodeId> children) {
    if (!leaves_.contains(parent))
        return false;
    for (const NodeId& c : children)
        if (!roots_.contains(c))
            return false;

    map_.insert({parent, children});
    log(srt_children_insert<NodeId>{parent, children});

    for (const NodeId& c : children) {
        parent_map_.insert({c, parent});
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
const std::unordered_set<NodeId>&
dbuct_series_reduced_tree<NodeId>::roots() const {
    return roots_;
}

template<typename NodeId>
const std::unordered_set<NodeId>&
dbuct_series_reduced_tree<NodeId>::leaves() const {
    return leaves_;
}

template<typename NodeId>
const std::set<NodeId>&
dbuct_series_reduced_tree<NodeId>::children(NodeId parent) const {
    return map_.at(parent);
}

template<typename NodeId>
void dbuct_series_reduced_tree<NodeId>::try_reduce(NodeId node) {
    auto it = map_.find(node);
    if (it == map_.end())
        return;
    const size_t child_count = it->second.size();
    if (child_count == 0)
        reduce_nullary(node);
    else if (child_count == 1)
        reduce_unary(node);
}

template<typename NodeId>
void dbuct_series_reduced_tree<NodeId>::reduce_nullary(NodeId node) {
    child_set_t captured = std::move(map_.at(node));
    map_.erase(node);
    log(srt_children_erase<NodeId>{node, std::move(captured)});

    auto grandparent_it = parent_map_.find(node);
    if (grandparent_it == parent_map_.end()) {
        roots_.erase(node);
        log(srt_set_erase<NodeId>{srt_set_target::roots, node});
    } else {
        const NodeId grandparent = grandparent_it->second;
        map_.at(grandparent).erase(node);
        log(srt_children_at_erase<NodeId>{grandparent, node});
        NodeId captured_parent = grandparent_it->second;
        parent_map_.erase(node);
        log(srt_parent_erase<NodeId>{node, captured_parent});
        try_reduce(grandparent);
    }
}

template<typename NodeId>
void dbuct_series_reduced_tree<NodeId>::reduce_unary(NodeId parent) {
    const NodeId child = *map_.at(parent).begin();

    child_set_t captured_children = std::move(map_.at(parent));
    map_.erase(parent);
    log(srt_children_erase<NodeId>{parent, std::move(captured_children)});

    auto grandparent_it = parent_map_.find(parent);
    if (grandparent_it == parent_map_.end()) {
        NodeId captured_parent = parent_map_.at(child);
        parent_map_.erase(child);
        log(srt_parent_erase<NodeId>{child, captured_parent});
        roots_.erase(parent);
        log(srt_set_erase<NodeId>{srt_set_target::roots, parent});
        roots_.insert(child);
        log(srt_set_insert<NodeId>{srt_set_target::roots, child});
    } else {
        const NodeId grandparent = grandparent_it->second;
        NodeId previous_parent = parent_map_.at(child);
        parent_map_.at(child) = grandparent;
        log(srt_parent_assign<NodeId>{child, previous_parent});
        map_.at(grandparent).erase(parent);
        log(srt_children_at_erase<NodeId>{grandparent, parent});
        map_.at(grandparent).insert(child);
        log(srt_children_at_insert<NodeId>{grandparent, child});
        parent_map_.erase(parent);
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
    for (auto it = current.actions_.rbegin(); it != current.actions_.rend(); ++it)
        undo_action(*it);
}

template<typename NodeId>
void dbuct_series_reduced_tree<NodeId>::log(srt_action<NodeId> action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions_.push_back(std::move(action));
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
        map_.erase(op->parent);
    } else if (const auto* op = std::get_if<srt_children_erase<NodeId>>(&action)) {
        map_.insert({op->parent, op->children});
    } else if (const auto* op = std::get_if<srt_parent_insert<NodeId>>(&action)) {
        parent_map_.erase(op->child);
    } else if (const auto* op = std::get_if<srt_parent_erase<NodeId>>(&action)) {
        parent_map_.insert({op->child, op->parent});
    } else if (const auto* op = std::get_if<srt_parent_assign<NodeId>>(&action)) {
        parent_map_.at(op->child) = op->previous;
    } else if (const auto* op = std::get_if<srt_children_at_insert<NodeId>>(&action)) {
        map_.at(op->parent).erase(op->child);
    } else if (const auto* op = std::get_if<srt_children_at_erase<NodeId>>(&action)) {
        map_.at(op->parent).insert(op->child);
    }
}

#endif
