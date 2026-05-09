#ifndef SIM_STARTER_INITIAL_GOALS_ACTIVATED_EVENT_HANDLER_HPP
#define SIM_STARTER_INITIAL_GOALS_ACTIVATED_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/initial_goals_activated_event.hpp"
#include "../../domain/interfaces/i_sim_starter.hpp"

struct sim_starter_initial_goals_activated_event_handler : event_handler<initial_goals_activated_event> {
    sim_starter_initial_goals_activated_event_handler();
    void handle(const initial_goals_activated_event&) override;
private:
    i_sim_starter& sim_starter;
};

#endif
