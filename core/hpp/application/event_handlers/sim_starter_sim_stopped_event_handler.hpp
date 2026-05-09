#ifndef SIM_STARTER_SIM_STOPPED_EVENT_HANDLER_HPP
#define SIM_STARTER_SIM_STOPPED_EVENT_HANDLER_HPP

#include "../../infrastructure/cancellable_event_handler.hpp"
#include "../../domain/events/sim_stopped_event.hpp"
#include "../../domain/events/refuted_event.hpp"
#include "../../domain/events/never_event.hpp"
#include "../../domain/interfaces/i_sim_starter.hpp"

struct sim_starter_sim_stopped_event_handler :
        cancellable_event_handler<sim_stopped_event, refuted_event, never_event> {
    sim_starter_sim_stopped_event_handler();
    void execute(const sim_stopped_event&) override;
private:
    i_sim_starter& sim_starter;
};

#endif
