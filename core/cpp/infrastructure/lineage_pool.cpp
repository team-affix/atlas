#include "infrastructure/lineage_pool.hpp"

const goal_lineage* lineage_pool::make_goal_lineage(const resolution_lineage* parent, subgoal_id idx) {
    return intern(goal_lineage{parent, idx});
}

const resolution_lineage* lineage_pool::make_resolution_lineage(const goal_lineage* parent, rule_id idx) {
    return intern(resolution_lineage{parent, idx});
}

void lineage_pool::pin(const goal_lineage* lineage) {
    if (!lineage)
        return;
    if (!pinned_goals_.insert(lineage).second)
        return;
    pin(lineage->parent);
}

void lineage_pool::pin(const resolution_lineage* lineage) {
    if (!lineage)
        return;
    if (!pinned_resolutions_.insert(lineage).second)
        return;
    pin(lineage->parent);
}

void lineage_pool::trim() {
    for (auto it = goal_lineages_.begin(); it != goal_lineages_.end();) {
        if (pinned_goals_.contains(&*it)) {
            ++it;
            continue;
        }
        it = goal_lineages_.erase(it);
    }
    for (auto it = resolution_lineages_.begin(); it != resolution_lineages_.end();) {
        if (pinned_resolutions_.contains(&*it)) {
            ++it;
            continue;
        }
        it = resolution_lineages_.erase(it);
    }
}

const goal_lineage* lineage_pool::import(const goal_lineage* lineage) {
    if (lineage == nullptr)
        return nullptr;
    return make_goal_lineage(import(lineage->parent), lineage->idx);
}

const resolution_lineage* lineage_pool::import(const resolution_lineage* lineage) {
    if (lineage == nullptr)
        return nullptr;
    return make_resolution_lineage(import(lineage->parent), lineage->idx);
}

const goal_lineage* lineage_pool::intern(goal_lineage&& lineage) {
    return &*goal_lineages_.insert(std::move(lineage)).first;
}

const resolution_lineage* lineage_pool::intern(resolution_lineage&& lineage) {
    return &*resolution_lineages_.insert(std::move(lineage)).first;
}
