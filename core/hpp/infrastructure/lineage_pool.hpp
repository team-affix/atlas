#ifndef LINEAGE_POOL_HPP
#define LINEAGE_POOL_HPP

#include <unordered_set>
#include "value_objects/goal_lineage_hash.hpp"
#include "value_objects/resolution_lineage_hash.hpp"
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
    std::unordered_set<goal_lineage, goal_lineage_hash> goal_lineages_;
    std::unordered_set<resolution_lineage, resolution_lineage_hash> resolution_lineages_;
    std::unordered_set<const goal_lineage*> pinned_goals_;
    std::unordered_set<const resolution_lineage*> pinned_resolutions_;
};

#endif
