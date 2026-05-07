#ifndef INITIAL_GOAL_WEIGHT_INITIALIZER_HPP
#define INITIAL_GOAL_WEIGHT_INITIALIZER_HPP

#include "../interfaces/i_initial_goal_weight_initializer.hpp"
#include "../interfaces/i_goal_weight_store.hpp"

struct initial_goal_weight_initializer : i_initial_goal_weight_initializer {
    initial_goal_weight_initializer(size_t goal_count);
    void initialize(const goal_lineage*) override;
private:
    i_goal_weight_store& gws;
    double initial_weight;
};

#endif
