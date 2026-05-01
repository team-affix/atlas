#ifndef GOAL_ACTIVATOR_HPP
#define GOAL_ACTIVATOR_HPP

#include "../interfaces/i_goal_activator.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/goal_activating_event.hpp"
#include "../events/goal_activated_event.hpp"

struct goal_activator : i_goal_activator {
    goal_activator();
    void activate(const goal_lineage*) override;
#ifndef DEBUG
private:
#endif
    i_event_producer<goal_activating_event>& goal_activating_producer;
    i_event_producer<goal_activated_event>& goal_activated_producer;
};

#endif
