#ifndef CANDIDATE_DEACTIVATOR_HPP
#define CANDIDATE_DEACTIVATOR_HPP

#include "interfaces/i_candidate_deactivator.hpp"
#include "interfaces/i_unset_candidate_translation_map.hpp"
#include "interfaces/i_deactivated_candidate_memory.hpp"
#include "interfaces/i_unlink_goal_candidate.hpp"

struct candidate_deactivator : i_candidate_deactivator {
    candidate_deactivator(
        i_unset_candidate_translation_map& unset_candidate_translation_map,
        i_deactivated_candidate_memory& deactivated_candidate_memory,
        i_unlink_goal_candidate& unlink_goal_candidate);
    void deactivate(const resolution_lineage*) override;
private:
    i_unset_candidate_translation_map& unset_candidate_translation_map;
    i_deactivated_candidate_memory& deactivated_candidate_memory;
    i_unlink_goal_candidate& unlink_goal_candidate;
};

#endif
