#ifndef SIM_STARTING_SIM_STARTER_EVENT_HANDLER_HPP
#define SIM_STARTING_SIM_STARTER_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/sim_starting_event.hpp"
#include "../../domain/interfaces/i_sim_starter.hpp"

struct sim_starting_sim_starter_event_handler : event_handler<sim_starting_event> {
    sim_starting_sim_starter_event_handler();
    void handle(const sim_starting_event&) override;
private:
    i_sim_starter& sim_starter;
};

#endif
