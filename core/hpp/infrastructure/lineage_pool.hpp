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

#endif
