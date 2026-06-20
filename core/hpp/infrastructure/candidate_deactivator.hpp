#ifndef CANDIDATE_DEACTIVATOR_HPP
#define CANDIDATE_DEACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename IUnsetCandidateFrameOffset, typename IUnlinkGoalCandidate>
struct candidate_deactivator {
    candidate_deactivator(IUnsetCandidateFrameOffset& cfo, IUnlinkGoalCandidate& gcr);
    void deactivate(const resolution_lineage*);
private:
    IUnsetCandidateFrameOffset& unset_candidate_frame_offset;
    IUnlinkGoalCandidate& unlink_goal_candidate;
};

template<typename IUCFO, typename IULGC>
candidate_deactivator<IUCFO, IULGC>::candidate_deactivator(IUCFO& cfo, IULGC& gcr)
    : unset_candidate_frame_offset(cfo), unlink_goal_candidate(gcr) {}

template<typename IUCFO, typename IULGC>
void candidate_deactivator<IUCFO, IULGC>::deactivate(const resolution_lineage* rl) {
    unlink_goal_candidate.unlink_goal_candidate(rl->parent, rl->idx);
    unset_candidate_frame_offset.unset(rl);
}

#endif
