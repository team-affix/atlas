#ifndef INITIAL_GOAL_WEIGHT_INITIALIZER_HPP
#define INITIAL_GOAL_WEIGHT_INITIALIZER_HPP

#include "../interfaces/i_initial_goal_weight_initializer.hpp"

struct initial_goal_weight_initializer : i_initial_goal_weight_initializer {
    initial_goal_weight_initializer();
    void initialize(const goal_lineage*) override;
};

#endif
