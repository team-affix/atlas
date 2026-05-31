#include "infrastructure/candidate_deactivator.hpp"

candidate_deactivator::candidate_deactivator(locator& loc)
    :
    unset_candidate_translation_map(loc.locate<i_unset_candidate_translation_map>()),
    unlink_goal_candidate(loc.locate<i_unlink_goal_candidate>()) {}

void candidate_deactivator::deactivate(const resolution_lineage* rl) {
    unlink_goal_candidate.unlink_goal_candidate(rl->parent, rl->idx);
    unset_candidate_translation_map.unset(rl);
}
