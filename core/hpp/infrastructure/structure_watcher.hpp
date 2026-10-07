#ifndef STRUCTURE_WATCHER_HPP
#define STRUCTURE_WATCHER_HPP

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <immer/map.hpp>
#include <immer/set.hpp>
#include "value_objects/pud_node.hpp"

using watcher_head_id = uint32_t;

template<typename IGetParentNode>
struct structure_watcher {
    structure_watcher(IGetParentNode& get_parent);
    void watch(const pud_node* node, watcher_head_id head, const std::vector<uint32_t>& reps);
    std::vector<watcher_head_id> note_var_bind(const pud_node* node, uint32_t bound, uint32_t target);
    std::vector<watcher_head_id> note_functor_bind(const pud_node* node, uint32_t bound, const std::vector<uint32_t>& introduced_reps);
    std::vector<watcher_head_id> heads_of(const pud_node* node, uint32_t rep) const;
    std::vector<uint32_t> reps_of(const pud_node* node, watcher_head_id head) const;

private:
    using head_set_t = immer::set<watcher_head_id>;
    using rep_set_t  = immer::set<uint32_t>;

    struct node_state {
        immer::map<uint32_t,        head_set_t> reps_to_heads;
        immer::map<watcher_head_id, rep_set_t>  heads_to_reps;
    };

    node_state current_or_inherited(const pud_node* node) const;
    head_set_t heads_in(const node_state& s, uint32_t rep) const;
    rep_set_t  reps_in(const node_state& s, watcher_head_id head) const;

    IGetParentNode& get_parent_;
    std::unordered_map<const pud_node*, node_state> states_;
};

template<typename IGetParentNode>
structure_watcher<IGetParentNode>::structure_watcher(IGetParentNode& get_parent)
    : get_parent_(get_parent)
    , states_() {}

template<typename IGetParentNode>
typename structure_watcher<IGetParentNode>::node_state
structure_watcher<IGetParentNode>::current_or_inherited(const pud_node* node) const {
    const pud_node* cur = node;
    while (cur != nullptr) {
        const auto it = states_.find(cur);
        if (it != states_.end())
            return it->second;
        cur = get_parent_.get(cur);
    }
    return node_state{};
}

template<typename IGetParentNode>
typename structure_watcher<IGetParentNode>::head_set_t
structure_watcher<IGetParentNode>::heads_in(const node_state& s, uint32_t rep) const {
    const auto* found = s.reps_to_heads.find(rep);
    return found ? *found : head_set_t{};
}

template<typename IGetParentNode>
typename structure_watcher<IGetParentNode>::rep_set_t
structure_watcher<IGetParentNode>::reps_in(const node_state& s, watcher_head_id head) const {
    const auto* found = s.heads_to_reps.find(head);
    return found ? *found : rep_set_t{};
}

template<typename IGetParentNode>
void structure_watcher<IGetParentNode>::watch(const pud_node* node,
                                              watcher_head_id head,
                                              const std::vector<uint32_t>& reps) {
    node_state s = current_or_inherited(node);

    rep_set_t rep_set = reps_in(s, head);
    for (uint32_t rep : reps)
        rep_set = rep_set.insert(rep);
    s.heads_to_reps = s.heads_to_reps.set(head, rep_set);

    for (uint32_t rep : reps) {
        head_set_t head_set = heads_in(s, rep);
        head_set = head_set.insert(head);
        s.reps_to_heads = s.reps_to_heads.set(rep, head_set);
    }

    states_.insert_or_assign(node, std::move(s));
}

template<typename IGetParentNode>
std::vector<watcher_head_id>
structure_watcher<IGetParentNode>::note_var_bind(const pud_node* node,
                                                 uint32_t bound, uint32_t target) {
    node_state s = current_or_inherited(node);

    const head_set_t bound_heads = heads_in(s, bound);

    std::vector<watcher_head_id> changed;
    head_set_t new_bound_heads = bound_heads;
    head_set_t new_target_heads = heads_in(s, target);

    for (watcher_head_id head : bound_heads) {
        rep_set_t head_reps = reps_in(s, head);
        const bool collapse = head_reps.count(target) != 0;

        head_reps = head_reps.erase(bound);
        if (!collapse)
            head_reps = head_reps.insert(target);
        s.heads_to_reps = s.heads_to_reps.set(head, head_reps);

        new_bound_heads = new_bound_heads.erase(head);
        if (!collapse)
            new_target_heads = new_target_heads.insert(head);

        if (collapse)
            changed.push_back(head);
    }

    s.reps_to_heads = s.reps_to_heads.set(bound, new_bound_heads);
    s.reps_to_heads = s.reps_to_heads.set(target, new_target_heads);

    states_.insert_or_assign(node, std::move(s));

    std::sort(changed.begin(), changed.end());
    return changed;
}

template<typename IGetParentNode>
std::vector<watcher_head_id>
structure_watcher<IGetParentNode>::note_functor_bind(const pud_node* node,
                                                     uint32_t bound,
                                                     const std::vector<uint32_t>& introduced_reps) {
    node_state s = current_or_inherited(node);

    const head_set_t bound_heads = heads_in(s, bound);

    std::vector<watcher_head_id> changed;
    for (watcher_head_id head : bound_heads)
        changed.push_back(head);

    for (watcher_head_id head : bound_heads) {
        rep_set_t head_reps = reps_in(s, head);
        head_reps = head_reps.erase(bound);
        for (uint32_t rep : introduced_reps)
            head_reps = head_reps.insert(rep);
        s.heads_to_reps = s.heads_to_reps.set(head, head_reps);
    }

    s.reps_to_heads = s.reps_to_heads.set(bound, head_set_t{});

    for (uint32_t rep : introduced_reps) {
        head_set_t rep_heads = heads_in(s, rep);
        for (watcher_head_id head : bound_heads)
            rep_heads = rep_heads.insert(head);
        s.reps_to_heads = s.reps_to_heads.set(rep, rep_heads);
    }

    states_.insert_or_assign(node, std::move(s));

    std::sort(changed.begin(), changed.end());
    return changed;
}

template<typename IGetParentNode>
std::vector<watcher_head_id>
structure_watcher<IGetParentNode>::heads_of(const pud_node* node, uint32_t rep) const {
    const node_state s = current_or_inherited(node);
    const auto* found = s.reps_to_heads.find(rep);
    std::vector<watcher_head_id> result;
    if (!found) return result;
    for (watcher_head_id h : *found)
        result.push_back(h);
    return result;
}

template<typename IGetParentNode>
std::vector<uint32_t>
structure_watcher<IGetParentNode>::reps_of(const pud_node* node, watcher_head_id head) const {
    const node_state s = current_or_inherited(node);
    const auto* found = s.heads_to_reps.find(head);
    std::vector<uint32_t> result;
    if (!found) return result;
    for (uint32_t r : *found)
        result.push_back(r);
    return result;
}

#endif
