#ifndef CANDIDATE_ACTIVATOR_HPP
#define CANDIDATE_ACTIVATOR_HPP

#include "../interfaces/i_goal_activator.hpp"

struct goal_activator : i_goal_activator {
    goal_activator();
    void activate(const goal_lineage*) override;
};

#endif
