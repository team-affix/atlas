#ifndef RESOLVER_HPP
#define RESOLVER_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_resolver.hpp"
#include "interfaces/i_goal_deactivator.hpp"
#include "interfaces/i_activate_subgoals_and_candidates.hpp"
#include "interfaces/i_deactivate_goal_candidates.hpp"

struct resolver : i_resolver {
    resolver(locator& loc);
    bool resolve(const resolution_lineage*) override;
private:
    i_goal_deactivator& goal_deactivator;
    i_activate_subgoals_and_candidates& activate_subgoals_and_candidates;
    i_deactivate_goal_candidates& deactivate_goal_candidates;
};

#endif
