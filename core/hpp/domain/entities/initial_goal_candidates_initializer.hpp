#ifndef INITIAL_GOAL_CANDIDATES_INITIALIZER_HPP
#define INITIAL_GOAL_CANDIDATES_INITIALIZER_HPP

#include "../interfaces/i_initial_goal_candidates_initializer.hpp"
#include "../interfaces/i_goal_candidates_store.hpp"
#include "../value_objects/candidate_set.hpp"

struct initial_goal_candidates_initializer : i_initial_goal_candidates_initializer {
    initial_goal_candidates_initializer();
    void initialize(const goal_lineage*) override;
private:
    i_goal_candidates_store& gcs;
    candidate_set initial_candidates;
};

#endif
