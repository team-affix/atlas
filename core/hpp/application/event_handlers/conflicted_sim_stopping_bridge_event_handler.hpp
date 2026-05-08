#ifndef CONFLICTED_SIM_STOPPING_BRIDGE_EVENT_HANDLER_HPP
#define CONFLICTED_SIM_STOPPING_BRIDGE_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/conflicted_event.hpp"
#include "../../domain/events/sim_stopping_event.hpp"
#include "../../domain/interfaces/i_event_producer.hpp"

struct conflicted_sim_stopping_bridge_event_handler : event_handler<conflicted_event> {
    conflicted_sim_stopping_bridge_event_handler();
    void handle(const conflicted_event&) override;
private:
    i_event_producer<sim_stopping_event>& sim_stopping_producer;
};

#endif
