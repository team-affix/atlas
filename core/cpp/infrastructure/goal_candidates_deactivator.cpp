#include "infrastructure/goal_candidates_deactivator.hpp"

goal_candidates_deactivator::goal_candidates_deactivator(locator& loc)
    :
    get_goal_candidate_rule_ids(loc.locate<i_get_goal_candidate_rule_ids>()),
    make_resolution_lineage(loc.locate<i_make_resolution_lineage>()),
    candidate_deactivator(loc.locate<i_candidate_deactivator>()) {}

void goal_candidates_deactivator::deactivate_goal_candidates(const goal_lineage* gl) {
    auto candidate_rules = get_goal_candidate_rule_ids.get(gl).copy();
    auto cand_it = candidate_rules->iterate();
    while (!cand_it.done()) {
        cand_it.resume();
        if (!cand_it.has_yield())
            continue;
        candidate_deactivator.deactivate(
            make_resolution_lineage.make_resolution_lineage(gl, cand_it.consume_yield()));
    }
}
