#ifndef INITIAL_GOAL_CANDIDATES_INITIALIZER_HPP
#define INITIAL_GOAL_CANDIDATES_INITIALIZER_HPP

#include "../interfaces/i_initial_goal_candidates_initializer.hpp"

struct initial_goal_candidates_initializer : i_initial_goal_candidates_initializer {
    initial_goal_candidates_initializer();
    void initialize(const goal_lineage*) override;
};

#endif
