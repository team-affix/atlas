#ifndef CANDIDATE_DEACTIVATOR_HPP
#define CANDIDATE_DEACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename ICandidateFrameOffsets, typename IGoalCandidateRules>
struct candidate_deactivator {
    candidate_deactivator(ICandidateFrameOffsets& cfo, IGoalCandidateRules& gcr);
    void deactivate(const resolution_lineage*);
private:
    ICandidateFrameOffsets& unset_candidate_frame_offset;
    IGoalCandidateRules& unlink_goal_candidate;
};

template<typename ICFO, typename IGCR>
candidate_deactivator<ICFO, IGCR>::candidate_deactivator(ICFO& cfo, IGCR& gcr)
    : unset_candidate_frame_offset(cfo), unlink_goal_candidate(gcr) {}

template<typename ICFO, typename IGCR>
void candidate_deactivator<ICFO, IGCR>::deactivate(const resolution_lineage* rl) {
    unlink_goal_candidate.unlink_goal_candidate(rl->parent, rl->idx);
    unset_candidate_frame_offset.unset(rl);
}

#endif
