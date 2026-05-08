#ifndef GOAL_STORES_CLEARED_SIM_STOPPER_EVENT_HANDLER_HPP
#define GOAL_STORES_CLEARED_SIM_STOPPER_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/goal_stores_cleared_event.hpp"
#include "../../domain/interfaces/i_sim_stopper.hpp"

struct goal_stores_cleared_sim_stopper_event_handler : event_handler<goal_stores_cleared_event> {
    goal_stores_cleared_sim_stopper_event_handler();
    void handle(const goal_stores_cleared_event&) override;
private:
    i_sim_stopper& sim_stopper;
};

#endif
