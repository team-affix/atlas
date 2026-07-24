#ifndef QUELL_GOAL_DEACTIVATOR_HPP
#define QUELL_GOAL_DEACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename ISrtGoalDeactivator, typename IEraseGoalDepth, typename IEraseGoalWorkValue>
struct quell_goal_deactivator {
    quell_goal_deactivator(ISrtGoalDeactivator&, IEraseGoalDepth&, IEraseGoalWorkValue&);
    void deactivate(const goal_lineage*);
private:
    ISrtGoalDeactivator& srt_goal_deactivator_;
    IEraseGoalDepth& erase_goal_depth_;
    IEraseGoalWorkValue& erase_goal_work_value_;
};

template<typename ISGD, typename IEGD, typename IEGWV>
quell_goal_deactivator<ISGD, IEGD, IEGWV>::quell_goal_deactivator(
    ISGD& sgd, IEGD& egd, IEGWV& egwv)
    : srt_goal_deactivator_(sgd)
    , erase_goal_depth_(egd)
    , erase_goal_work_value_(egwv) {}

template<typename ISGD, typename IEGD, typename IEGWV>
void quell_goal_deactivator<ISGD, IEGD, IEGWV>::deactivate(const goal_lineage* gl) {
    erase_goal_depth_.erase(gl);
    erase_goal_work_value_.erase(gl);
    srt_goal_deactivator_.deactivate(gl);
}

#endif
