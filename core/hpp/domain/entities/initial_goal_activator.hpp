#ifndef INITIAL_GOAL_ACTIVATOR_HPP
#define INITIAL_GOAL_ACTIVATOR_HPP

#include "../interfaces/i_initial_goal_activator.hpp"
#include "../interfaces/i_lineage_pool.hpp"
#include "../events/initial_goal_activating_event.hpp"
#include "../events/initial_goals_activated_event.hpp"
#include "../events/goal_activated_event.hpp"
#include "../interfaces/i_event_producer.hpp"

struct initial_goal_activator : i_initial_goal_activator {
    initial_goal_activator(size_t);
    void activate_initial_goals() override;
private:
    i_lineage_pool& lineage_pool;
    i_event_producer<initial_goal_activating_event>& initial_goal_activating_producer;
    i_event_producer<goal_activated_event>& goal_activated_producer;
    i_event_producer<initial_goals_activated_event>& initial_goals_activated_producer;

    size_t initial_goal_count;
};

#endif
