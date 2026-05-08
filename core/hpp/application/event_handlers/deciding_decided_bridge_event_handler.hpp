#ifndef DECIDING_DECIDED_BRIDGE_EVENT_HANDLER_HPP
#define DECIDING_DECIDED_BRIDGE_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/deciding_event.hpp"
#include "../../domain/events/decided_event.hpp"
#include "../../domain/interfaces/i_event_producer.hpp"

struct deciding_decided_bridge_event_handler : event_handler<deciding_event> {
    deciding_decided_bridge_event_handler();
    void handle(const deciding_event&) override;
private:
    i_event_producer<decided_event>& decided_producer;
};

#endif
