#ifndef SIM_STOPPER_CONFLICTED_EVENT_HANDLER_HPP
#define SIM_STOPPER_CONFLICTED_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/conflicted_event.hpp"
#include "../../domain/interfaces/i_sim_stopper.hpp"

struct sim_stopper_conflicted_event_handler : event_handler<conflicted_event> {
    sim_stopper_conflicted_event_handler();
    void handle(const conflicted_event&) override;
private:
    i_sim_stopper& sim_stopper;
};

#endif
