#ifndef CANDIDATE_DEACTIVATOR_HPP
#define CANDIDATE_DEACTIVATOR_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_candidate_deactivator.hpp"
#include "interfaces/i_unset_candidate_frame_offset.hpp"
#include "interfaces/i_unlink_goal_candidate.hpp"

struct candidate_deactivator : i_candidate_deactivator {
    candidate_deactivator(locator& loc);
    void deactivate(const resolution_lineage*) override;
private:
    i_unset_candidate_frame_offset& unset_candidate_frame_offset;
    i_unlink_goal_candidate& unlink_goal_candidate;
};

#endif
