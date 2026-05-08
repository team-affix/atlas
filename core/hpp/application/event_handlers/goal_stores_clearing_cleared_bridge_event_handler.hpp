#ifndef GOAL_STORES_CLEARING_CLEARED_BRIDGE_EVENT_HANDLER_HPP
#define GOAL_STORES_CLEARING_CLEARED_BRIDGE_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/goal_stores_clearing_event.hpp"
#include "../../domain/events/goal_stores_cleared_event.hpp"
#include "../../domain/interfaces/i_event_producer.hpp"

struct goal_stores_clearing_cleared_bridge_event_handler : event_handler<goal_stores_clearing_event> {
    goal_stores_clearing_cleared_bridge_event_handler();
    void handle(const goal_stores_clearing_event&) override;
private:
    i_event_producer<goal_stores_cleared_event>& goal_stores_cleared_producer;
};

#endif
