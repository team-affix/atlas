#include "../../../hpp/domain/data_structures/lineage_pool.hpp"

const goal_lineage* lineage_pool::goal(const resolution_lineage* parent, size_t idx) {
    return intern(goal_lineage{parent, idx});
}

const resolution_lineage* lineage_pool::resolution(const goal_lineage* parent, size_t idx) {
    return intern(resolution_lineage{parent, idx});
}

void lineage_pool::pin(const goal_lineage* l) {
    if (!l)
        return; /*at root*/
    bool& is_pinned = goal_lineages.at(*l);
    if (is_pinned)
        return; /*from here to root is already pinned*/
    is_pinned = true;
    pin(l->parent); /*pin from here to root*/
}

void lineage_pool::pin(const resolution_lineage* l) {
    if (!l)
        return; /*at root*/
    bool& is_pinned = resolution_lineages.at(*l);
    if (is_pinned)
        return; /*from here to root is already pinned*/
    is_pinned = true;
    pin(l->parent); /*pin from here to root*/
}

void lineage_pool::trim() {
    for (auto it = goal_lineages.begin(); it != goal_lineages.end();) {
        if (it->second) { ++it; continue; }
        goal_lineages.erase(it++); // remove unpinned goal lineages
    }
    for (auto it = resolution_lineages.begin(); it != resolution_lineages.end();) {
        if (it->second) { ++it; continue; }
        resolution_lineages.erase(it++); // remove unpinned resolution lineages
    }
}

const goal_lineage* lineage_pool::import(const goal_lineage* l) {
    if (l == nullptr)
        return nullptr;
    return goal(import(l->parent), l->idx);
}

const resolution_lineage* lineage_pool::import(const resolution_lineage* l) {
    if (l == nullptr)
        return nullptr;
    return resolution(import(l->parent), l->idx);
}

const goal_lineage* lineage_pool::intern(goal_lineage&& l) {
    return &goal_lineages.emplace(std::move(l), false).first->first;
}

const resolution_lineage* lineage_pool::intern(resolution_lineage&& l) {
    return &resolution_lineages.emplace(std::move(l), false).first->first;
}
