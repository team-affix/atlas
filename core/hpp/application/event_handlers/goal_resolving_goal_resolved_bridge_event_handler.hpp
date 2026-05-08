#ifndef GOAL_RESOLVING_GOAL_RESOLVED_BRIDGE_EVENT_HANDLER_HPP
#define GOAL_RESOLVING_GOAL_RESOLVED_BRIDGE_EVENT_HANDLER_HPP

#include "../../infrastructure/cancellable_event_handler.hpp"
#include "../../domain/events/goal_resolving_event.hpp"
#include "../../domain/events/goal_resolved_event.hpp"
#include "../../domain/events/sim_cancelled_event.hpp"
#include "../../domain/events/sim_cancellation_reset_event.hpp"
#include "../../domain/interfaces/i_event_producer.hpp"

struct goal_resolving_goal_resolved_bridge_event_handler : cancellable_event_handler<goal_resolving_event, sim_cancelled_event, sim_cancellation_reset_event> {
    goal_resolving_goal_resolved_bridge_event_handler();
    void execute(const goal_resolving_event& event) override;
private:
    i_event_producer<goal_resolved_event>& goal_resolved_producer;
};

#endif
