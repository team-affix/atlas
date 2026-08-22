#ifndef GOAL_WORK_FUNCTION_HPP
#define GOAL_WORK_FUNCTION_HPP

#include <cmath>
#include "value_objects/lineage.hpp"

template<typename IGetGoalDepth>
struct goal_work_function {
    goal_work_function(IGetGoalDepth& get_goal_depth, double k, double j);
    double get(const goal_lineage* gl) const;
private:
    IGetGoalDepth& get_goal_depth_;
    double k_;
    double j_;
};

template<typename IGGD>
goal_work_function<IGGD>::goal_work_function(IGGD& get_goal_depth, double k, double j)
    : get_goal_depth_(get_goal_depth)
    , k_(k)
    , j_(j) {}

template<typename IGGD>
double goal_work_function<IGGD>::get(const goal_lineage* gl) const {
    const double depth = static_cast<double>(get_goal_depth_.get(gl));
    return 1.0 + std::exp(-k_ * (depth - j_));
}

#endif
