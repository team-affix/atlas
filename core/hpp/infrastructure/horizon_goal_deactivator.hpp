#ifndef HORIZON_GOAL_DEACTIVATOR_HPP
#define HORIZON_GOAL_DEACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename ISrtGoalDeactivator, typename IEraseGoalWeight>
struct horizon_goal_deactivator {
    horizon_goal_deactivator(ISrtGoalDeactivator&, IEraseGoalWeight&);
    void deactivate(const goal_lineage*);
private:
    ISrtGoalDeactivator& srt_goal_deactivator_;
    IEraseGoalWeight& goal_weights_;
};

template<typename ISGD, typename IEGW>
horizon_goal_deactivator<ISGD,IEGW>::horizon_goal_deactivator(ISGD& sgd, IEGW& gw)
    : srt_goal_deactivator_(sgd), goal_weights_(gw) {}

template<typename ISGD, typename IEGW>
void horizon_goal_deactivator<ISGD,IEGW>::deactivate(const goal_lineage* gl) {
    goal_weights_.erase(gl);
    srt_goal_deactivator_.deactivate(gl);
}

#endif
