#include "infrastructure/lineage_pool.hpp"

const goal_lineage* lineage_pool::make_goal_lineage(const resolution_lineage* parent, subgoal_id idx) {
    return intern(goal_lineage{parent, idx});
}

const resolution_lineage* lineage_pool::make_resolution_lineage(const goal_lineage* parent, rule_id idx) {
    return intern(resolution_lineage{parent, idx});
}

void lineage_pool::pin(const goal_lineage* l) {
    if (!l) return;
    bool& is_pinned = goal_lineages.at(*l);
    if (is_pinned) return;
    is_pinned = true;
    pin(l->parent);
}

void lineage_pool::pin(const resolution_lineage* l) {
    if (!l) return;
    bool& is_pinned = resolution_lineages.at(*l);
    if (is_pinned) return;
    is_pinned = true;
    pin(l->parent);
}

void lineage_pool::trim() {
    for (auto it = goal_lineages.begin(); it != goal_lineages.end();) {
        if (it->second) { ++it; continue; }
        goal_lineages.erase(it++);
    }
    for (auto it = resolution_lineages.begin(); it != resolution_lineages.end();) {
        if (it->second) { ++it; continue; }
        resolution_lineages.erase(it++);
    }
}

const goal_lineage* lineage_pool::import(const goal_lineage* l) {
    if (l == nullptr) return nullptr;
    return make_goal_lineage(import(l->parent), l->idx);
}

const resolution_lineage* lineage_pool::import(const resolution_lineage* l) {
    if (l == nullptr) return nullptr;
    return make_resolution_lineage(import(l->parent), l->idx);
}

const goal_lineage* lineage_pool::intern(goal_lineage&& l) {
    return &goal_lineages.emplace(std::move(l), false).first->first;
}

const resolution_lineage* lineage_pool::intern(resolution_lineage&& l) {
    return &resolution_lineages.emplace(std::move(l), false).first->first;
}
