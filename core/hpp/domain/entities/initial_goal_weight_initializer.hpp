#ifndef INITIAL_GOAL_WEIGHT_INITIALIZER_HPP
#define INITIAL_GOAL_WEIGHT_INITIALIZER_HPP

#include "../interfaces/i_initial_goal_weight_initializer.hpp"
#include "../interfaces/i_weight_frontier.hpp"

struct initial_goal_weight_initializer : i_initial_goal_weight_initializer {
    initial_goal_weight_initializer(size_t goal_count);
    void initialize(const goal_lineage*) override;
private:
    i_weight_frontier& gws;
    double initial_weight;
};

#endif
