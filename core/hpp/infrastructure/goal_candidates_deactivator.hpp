#ifndef GOAL_CANDIDATES_DEACTIVATOR_HPP
#define GOAL_CANDIDATES_DEACTIVATOR_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_deactivate_goal_candidates.hpp"
#include "interfaces/i_get_goal_candidate_rule_ids.hpp"
#include "interfaces/i_make_resolution_lineage.hpp"
#include "interfaces/i_candidate_deactivator.hpp"

struct goal_candidates_deactivator : i_deactivate_goal_candidates {
    goal_candidates_deactivator(locator& loc);
    void deactivate_goal_candidates(const goal_lineage*) override;
private:
    i_get_goal_candidate_rule_ids& get_goal_candidate_rule_ids;
    i_make_resolution_lineage& make_resolution_lineage;
    i_candidate_deactivator& candidate_deactivator;
};

#endif
