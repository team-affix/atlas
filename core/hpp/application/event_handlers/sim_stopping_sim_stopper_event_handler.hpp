#ifndef SIM_STOPPING_SIM_STOPPER_EVENT_HANDLER_HPP
#define SIM_STOPPING_SIM_STOPPER_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/sim_stopping_event.hpp"
#include "../../domain/interfaces/i_sim_stopper.hpp"

struct sim_stopping_sim_stopper_event_handler : event_handler<sim_stopping_event> {
    sim_stopping_sim_stopper_event_handler();
    void handle(const sim_stopping_event&) override;
private:
    i_sim_stopper& sim_stopper;
};

#endif
