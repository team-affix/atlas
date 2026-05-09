#ifndef SIM_STOPPER_SOLVED_EVENT_HANDLER_HPP
#define SIM_STOPPER_SOLVED_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/solved_event.hpp"
#include "../../domain/interfaces/i_sim_stopper.hpp"

struct sim_stopper_solved_event_handler : event_handler<solved_event> {
    sim_stopper_solved_event_handler();
    void handle(const solved_event&) override;
private:
    i_sim_stopper& sim_stopper;
};

#endif
