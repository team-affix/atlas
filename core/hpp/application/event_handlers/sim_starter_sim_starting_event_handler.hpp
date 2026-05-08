#ifndef SIM_STARTER_SIM_STARTING_EVENT_HANDLER_HPP
#define SIM_STARTER_SIM_STARTING_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/sim_starting_event.hpp"
#include "../../domain/interfaces/i_sim_starter.hpp"

struct sim_starter_sim_starting_event_handler : event_handler<sim_starting_event> {
    sim_starter_sim_starting_event_handler();
    void handle(const sim_starting_event&) override;
private:
    i_sim_starter& sim_starter;
};

#endif
