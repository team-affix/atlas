#ifndef SOLVED_SIM_TERMINATION_CONDITION_REACHED_BRIDGE_EVENT_HANDLER_HPP
#define SOLVED_SIM_TERMINATION_CONDITION_REACHED_BRIDGE_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/solved_event.hpp"
#include "../../domain/events/sim_termination_condition_reached_event.hpp"
#include "../../domain/interfaces/i_event_producer.hpp"

struct solved_sim_termination_condition_reached_bridge_event_handler : event_handler<solved_event> {
    solved_sim_termination_condition_reached_bridge_event_handler();
    void handle(const solved_event&) override;
private:
    i_event_producer<sim_termination_condition_reached_event>& producer;
};

#endif
