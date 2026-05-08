#ifndef SIM_STOPPING_SIM_CANCELLED_BRIDGE_EVENT_HANDLER_HPP
#define SIM_STOPPING_SIM_CANCELLED_BRIDGE_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/sim_stopping_event.hpp"
#include "../../domain/events/sim_cancelled_event.hpp"
#include "../../domain/interfaces/i_event_producer.hpp"

struct sim_stopping_sim_cancelled_bridge_event_handler : event_handler<sim_stopping_event> {
    sim_stopping_sim_cancelled_bridge_event_handler();
    void handle(const sim_stopping_event&) override;
private:
    i_event_producer<sim_cancelled_event>& sim_cancelled_producer;
};

#endif
