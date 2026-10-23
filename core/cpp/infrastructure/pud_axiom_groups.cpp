#include "infrastructure/pud_axiom_groups.hpp"
#include <algorithm>
#include <unordered_set>
#include "debug_assert.hpp"

pud_axiom_groups::pud_axiom_groups()
    : next_group_id_(0)
    , dependencies_()
    , members_()
    , node_groups_()
    , visible_roots_cache_() {}

pud_axiom_group_id pud_axiom_groups::create_group() {
    const pud_axiom_group_id group{.value = next_group_id_++};
    dependencies_.emplace_back();
    members_.emplace_back();
    visible_roots_cache_.emplace_back();
    visible_roots_ready_.push_back(false);
    return group;
}

void pud_axiom_groups::add_dependency(pud_axiom_group_id dependent, pud_axiom_group_id dependency) {
    DEBUG_ASSERT(dependent.value < next_group_id_);
    DEBUG_ASSERT(dependency.value < next_group_id_);
    std::vector<pud_axiom_group_id>& deps = dependencies_.at(dependent.value);
    if (std::ranges::find(deps, dependency) != deps.end())
        return;
    deps.push_back(dependency);
    std::unordered_set<size_t> visiting;
    std::vector<size_t> stack{dependency.value};
    while (!stack.empty()) {
        const size_t group = stack.back();
        stack.pop_back();
        if (group == dependent.value) {
            deps.pop_back();
            DEBUG_ASSERT(false);
            return;
        }
        if (!visiting.insert(group).second)
            continue;
        for (pud_axiom_group_id dep : dependencies_.at(group))
            stack.push_back(dep.value);
    }
    invalidate_visible_roots_cache();
}

void pud_axiom_groups::add_member(pud_axiom_group_id group, pud_node_id root) {
    DEBUG_ASSERT(group.value < next_group_id_);
    members_.at(group.value).push_back(root);
    invalidate_visible_roots_cache();
}

void pud_axiom_groups::store_node_group(pud_node_id node, pud_axiom_group_id group) {
    node_groups_.store(node, group);
}

pud_axiom_group_id pud_axiom_groups::node_group(pud_node_id node) const {
    return node_groups_.get(node);
}

const std::vector<pud_node_id>& pud_axiom_groups::axiom_roots(pud_node_id query_leaf) const {
    return visible_roots_for(node_group(query_leaf));
}

void pud_axiom_groups::invalidate_visible_roots_cache() {
    for (size_t index = 0; index < visible_roots_cache_.size(); ++index) {
        visible_roots_cache_.at(index).clear();
        visible_roots_ready_.at(index) = false;
    }
}

const std::vector<pud_node_id>& pud_axiom_groups::visible_roots_for(pud_axiom_group_id group) const {
    DEBUG_ASSERT(group.value < visible_roots_cache_.size());
    std::vector<pud_node_id>& cached = visible_roots_cache_.at(group.value);
    if (visible_roots_ready_.at(group.value))
        return cached;
    std::unordered_set<size_t> visible;
    std::vector<size_t> stack{group.value};
    while (!stack.empty()) {
        const size_t current = stack.back();
        stack.pop_back();
        if (!visible.insert(current).second)
            continue;
        for (pud_axiom_group_id dep : dependencies_.at(current))
            stack.push_back(dep.value);
    }
    std::vector<size_t> indegree(next_group_id_, 0);
    for (size_t g : visible) {
        for (pud_axiom_group_id dep : dependencies_.at(g)) {
            if (visible.find(dep.value) != visible.end())
                indegree.at(g)++;
        }
    }
    std::vector<size_t> ready;
    for (size_t g : visible) {
        if (indegree.at(g) == 0)
            ready.push_back(g);
    }
    std::vector<size_t> ordered;
    while (!ready.empty()) {
        const size_t g = ready.back();
        ready.pop_back();
        ordered.push_back(g);
        for (size_t other : visible) {
            if (other == g)
                continue;
            const std::vector<pud_axiom_group_id>& deps = dependencies_.at(other);
            if (std::ranges::find(deps, pud_axiom_group_id{.value = g}) == deps.end())
                continue;
            indegree.at(other)--;
            if (indegree.at(other) == 0)
                ready.push_back(other);
        }
    }
    DEBUG_ASSERT(ordered.size() == visible.size());
    for (size_t g : ordered) {
        const std::vector<pud_node_id>& group_members = members_.at(g);
        cached.insert(cached.end(), group_members.begin(), group_members.end());
    }
    visible_roots_ready_.at(group.value) = true;
    return cached;
}
