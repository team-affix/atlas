#ifndef RESOLVER_HPP
#define RESOLVER_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_resolver.hpp"
#include "interfaces/i_make_resolution_lineage.hpp"
#include "interfaces/i_goal_deactivator.hpp"
#include "interfaces/i_get_goal_candidate_rule_ids.hpp"
#include "interfaces/i_candidate_deactivator.hpp"
#include "interfaces/i_activate_subgoals.hpp"

struct resolver : i_resolver {
    resolver(locator& loc);
    bool resolve(const resolution_lineage*) override;
private:
    i_make_resolution_lineage& make_resolution_lineage;
    i_goal_deactivator& goal_deactivator;
    i_get_goal_candidate_rule_ids& get_goal_candidate_rule_ids;
    i_candidate_deactivator& candidate_deactivator;
    i_activate_subgoals& activate_subgoals;
};

#endif
