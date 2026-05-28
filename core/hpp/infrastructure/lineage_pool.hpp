#ifndef LINEAGE_POOL_HPP
#define LINEAGE_POOL_HPP

#include <map>
#include "../interfaces/i_import_goal_lineage.hpp"
#include "../interfaces/i_import_resolution_lineage.hpp"
#include "../interfaces/i_make_goal_lineage.hpp"
#include "../interfaces/i_make_resolution_lineage.hpp"
#include "../interfaces/i_pin_goal_lineage.hpp"
#include "../interfaces/i_pin_resolution_lineage.hpp"
#include "../interfaces/i_trim_unpinned_lineages.hpp"

struct lineage_pool
    : i_make_goal_lineage
    , i_make_resolution_lineage
    , i_pin_goal_lineage
    , i_pin_resolution_lineage
    , i_trim_unpinned_lineages
    , i_import_goal_lineage
    , i_import_resolution_lineage {
    const goal_lineage* make(const resolution_lineage*, subgoal_id idx) override;
    const resolution_lineage* make(const goal_lineage*, rule_id idx) override;
    void pin(const goal_lineage*) override;
    void pin(const resolution_lineage*) override;
    void trim() override;
    const goal_lineage* import(const goal_lineage*) override;
    const resolution_lineage* import(const resolution_lineage*) override;
private:
    const goal_lineage* intern(goal_lineage&&);
    const resolution_lineage* intern(resolution_lineage&&);
    std::map<goal_lineage, bool> goal_lineages;
    std::map<resolution_lineage, bool> resolution_lineages;
};

#endif
