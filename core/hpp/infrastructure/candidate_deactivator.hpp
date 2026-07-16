#ifndef CANDIDATE_DEACTIVATOR_HPP
#define CANDIDATE_DEACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename IUnsetCandidateFrameOffset, typename IUnlinkGoalCandidate>
struct candidate_deactivator {
    candidate_deactivator(IUnsetCandidateFrameOffset& cfo, IUnlinkGoalCandidate& gcr);
    void deactivate(const resolution_lineage*);
private:
    IUnsetCandidateFrameOffset& unset_candidate_frame_offset_;
    IUnlinkGoalCandidate& unlink_goal_candidate_;
};

template<typename IUCFO, typename IULGC>
candidate_deactivator<IUCFO, IULGC>::candidate_deactivator(IUCFO& cfo, IULGC& gcr)
    : unset_candidate_frame_offset_(cfo), unlink_goal_candidate_(gcr) {}

template<typename IUCFO, typename IULGC>
void candidate_deactivator<IUCFO, IULGC>::deactivate(const resolution_lineage* rl) {
    unlink_goal_candidate_.unlink_goal_candidate(rl->parent, rl->idx);
    unset_candidate_frame_offset_.unset(rl);
}

#endif
