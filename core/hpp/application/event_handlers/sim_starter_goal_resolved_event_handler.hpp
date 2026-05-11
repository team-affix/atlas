#ifndef SIM_STARTER_GOAL_RESOLVED_EVENT_HANDLER_HPP
#define SIM_STARTER_GOAL_RESOLVED_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/goal_resolved_event.hpp"
#include "../../domain/interfaces/i_sim_starter.hpp"

struct sim_starter_goal_resolved_event_handler : event_handler<goal_resolved_event> {
    sim_starter_goal_resolved_event_handler();
    void handle(const goal_resolved_event&) override;
private:
    i_sim_starter& sim_starter;
};

#endif
