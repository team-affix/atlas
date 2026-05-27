#include "../../hpp/infrastructure/candidate_deactivator.hpp"

candidate_deactivator::candidate_deactivator(
    i_unset_candidate_translation_map& unset_candidate_translation_map,
    i_deactivated_candidate_memory& deactivated_candidate_memory,
    i_unlink_goal_candidate& unlink_goal_candidate)
    :
    unset_candidate_translation_map(unset_candidate_translation_map),
    deactivated_candidate_memory(deactivated_candidate_memory),
    unlink_goal_candidate(unlink_goal_candidate) {}

void candidate_deactivator::deactivate(const resolution_lineage* rl) {
    unlink_goal_candidate.unlink_goal_candidate(rl->parent, rl->idx);
    unset_candidate_translation_map.unset(rl);
    deactivated_candidate_memory.insert(rl);
}
