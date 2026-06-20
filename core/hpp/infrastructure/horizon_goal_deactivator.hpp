#ifndef HORIZON_GOAL_DEACTIVATOR_HPP
#define HORIZON_GOAL_DEACTIVATOR_HPP

#include "value_objects/lineage.hpp"

template<typename ISrtGoalDeactivator, typename IGoalWeights>
struct horizon_goal_deactivator {
    horizon_goal_deactivator(ISrtGoalDeactivator&, IGoalWeights&);
    void deactivate(const goal_lineage*);
private:
    ISrtGoalDeactivator& srt_goal_deactivator_;
    IGoalWeights& goal_weights_;
};

template<typename ISGD, typename IGW>
horizon_goal_deactivator<ISGD,IGW>::horizon_goal_deactivator(ISGD& sgd, IGW& gw)
    : srt_goal_deactivator_(sgd), goal_weights_(gw) {}

template<typename ISGD, typename IGW>
void horizon_goal_deactivator<ISGD,IGW>::deactivate(const goal_lineage* gl) {
    goal_weights_.erase(gl);
    srt_goal_deactivator_.deactivate(gl);
}

#endif
