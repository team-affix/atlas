#ifndef LINEAGE_POOL_HPP
#define LINEAGE_POOL_HPP

#include <map>
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

struct lineage_pool {
    const goal_lineage* make_goal_lineage(const resolution_lineage*, subgoal_id idx);
    const resolution_lineage* make_resolution_lineage(const goal_lineage*, rule_id idx);
    void pin(const goal_lineage*);
    void pin(const resolution_lineage*);
    void trim();
    const goal_lineage* import(const goal_lineage*);
    const resolution_lineage* import(const resolution_lineage*);
private:
    const goal_lineage* intern(goal_lineage&&);
    const resolution_lineage* intern(resolution_lineage&&);
    std::map<goal_lineage, bool> goal_lineages;
    std::map<resolution_lineage, bool> resolution_lineages;
};

inline const goal_lineage* lineage_pool::make_goal_lineage(const resolution_lineage* parent, subgoal_id idx) {
    return intern(goal_lineage{parent, idx});
}

inline const resolution_lineage* lineage_pool::make_resolution_lineage(const goal_lineage* parent, rule_id idx) {
    return intern(resolution_lineage{parent, idx});
}

inline void lineage_pool::pin(const goal_lineage* l) {
    if (!l) return;
    bool& is_pinned = goal_lineages.at(*l);
    if (is_pinned) return;
    is_pinned = true;
    pin(l->parent);
}

inline void lineage_pool::pin(const resolution_lineage* l) {
    if (!l) return;
    bool& is_pinned = resolution_lineages.at(*l);
    if (is_pinned) return;
    is_pinned = true;
    pin(l->parent);
}

inline void lineage_pool::trim() {
    for (auto it = goal_lineages.begin(); it != goal_lineages.end();) {
        if (it->second) { ++it; continue; }
        goal_lineages.erase(it++);
    }
    for (auto it = resolution_lineages.begin(); it != resolution_lineages.end();) {
        if (it->second) { ++it; continue; }
        resolution_lineages.erase(it++);
    }
}

inline const goal_lineage* lineage_pool::import(const goal_lineage* l) {
    if (l == nullptr) return nullptr;
    return make_goal_lineage(import(l->parent), l->idx);
}

inline const resolution_lineage* lineage_pool::import(const resolution_lineage* l) {
    if (l == nullptr) return nullptr;
    return make_resolution_lineage(import(l->parent), l->idx);
}

inline const goal_lineage* lineage_pool::intern(goal_lineage&& l) {
    return &goal_lineages.emplace(std::move(l), false).first->first;
}

inline const resolution_lineage* lineage_pool::intern(resolution_lineage&& l) {
    return &resolution_lineages.emplace(std::move(l), false).first->first;
}

#endif
