#ifndef GOAL_CANDIDATES_ACTIVATOR_GOAL_ACTIVATING_EVENT_HANDLER_HPP
#define GOAL_CANDIDATES_ACTIVATOR_GOAL_ACTIVATING_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/goal_activating_event.hpp"
#include "../../domain/interfaces/i_goal_candidates_activator.hpp"

struct goal_candidates_activator_goal_activating_event_handler : event_handler<goal_activating_event> {
    goal_candidates_activator_goal_activating_event_handler();
    void handle(const goal_activating_event&) override;
private:
    i_goal_candidates_activator& goal_candidates_activator;
};

#endif
