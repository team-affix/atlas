#ifndef GOAL_STORES_CLEARING_CLEARED_BRIDGE_EVENT_HANDLER_HPP
#define GOAL_STORES_CLEARING_CLEARED_BRIDGE_EVENT_HANDLER_HPP

#include "../../infrastructure/cancellable_event_handler.hpp"
#include "../../domain/events/goal_stores_clearing_event.hpp"
#include "../../domain/events/goal_stores_cleared_event.hpp"
#include "../../domain/events/sim_cancelled_event.hpp"
#include "../../domain/events/sim_cancellation_reset_event.hpp"
#include "../../domain/interfaces/i_event_producer.hpp"

struct goal_stores_clearing_cleared_bridge_event_handler : cancellable_event_handler<goal_stores_clearing_event, sim_cancelled_event, sim_cancellation_reset_event> {
    goal_stores_clearing_cleared_bridge_event_handler();
    void execute(const goal_stores_clearing_event&) override;
private:
    i_event_producer<goal_stores_cleared_event>& goal_stores_cleared_producer;
};

#endif
