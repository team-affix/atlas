#ifndef SIM_STOPPED_SIM_CANCELLATION_RESET_BRIDGE_EVENT_HANDLER_HPP
#define SIM_STOPPED_SIM_CANCELLATION_RESET_BRIDGE_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/sim_stopped_event.hpp"
#include "../../domain/events/sim_cancellation_reset_event.hpp"
#include "../../domain/interfaces/i_event_producer.hpp"

struct sim_stopped_sim_cancellation_reset_bridge_event_handler : event_handler<sim_stopped_event> {
    sim_stopped_sim_cancellation_reset_bridge_event_handler();
    void handle(const sim_stopped_event&) override;
private:
    i_event_producer<sim_cancellation_reset_event>& sim_cancellation_reset_producer;
};

#endif
