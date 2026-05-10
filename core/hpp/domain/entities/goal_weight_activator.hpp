#ifndef GOAL_WEIGHT_ACTIVATOR_HPP
#define GOAL_WEIGHT_ACTIVATOR_HPP

#include "../interfaces/i_goal_weight_activator.hpp"
#include "../interfaces/i_weight_frontier.hpp"
#include "../interfaces/i_database.hpp"

struct goal_weight_activator : i_goal_weight_activator {
    explicit goal_weight_activator(size_t initial_goal_count);
    void start_resolution(const resolution_lineage*) override;
    void activate(const goal_lineage*) override;
private:
    i_weight_frontier& wf;
    i_database& db;
    double initial_weight;
    double current_weight;
};

#endif
