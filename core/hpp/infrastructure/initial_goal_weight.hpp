#ifndef INITIAL_GOAL_WEIGHT_HPP
#define INITIAL_GOAL_WEIGHT_HPP

#include "interfaces/i_get_initial_goal_weight.hpp"

struct initial_goal_weight : i_get_initial_goal_weight {
    explicit initial_goal_weight(double weight);
    double get() const override;
private:
    double weight_;
};

#endif
