#ifndef GOAL_ACTIVATING_GOAL_ACTIVATED_BRIDGE_EVENT_HANDLER_HPP
#define GOAL_ACTIVATING_GOAL_ACTIVATED_BRIDGE_EVENT_HANDLER_HPP

#include "../../infrastructure/cancellable_event_handler.hpp"
#include "../../domain/events/goal_activating_event.hpp"
#include "../../domain/events/goal_activated_event.hpp"
#include "../../domain/events/sim_cancelled_event.hpp"
#include "../../domain/events/sim_cancellation_reset_event.hpp"
#include "../../domain/interfaces/i_event_producer.hpp"

struct goal_activating_goal_activated_bridge_event_handler : cancellable_event_handler<goal_activating_event, sim_cancelled_event, sim_cancellation_reset_event> {
    goal_activating_goal_activated_bridge_event_handler();
    void execute(const goal_activating_event& event) override;
private:
    i_event_producer<goal_activated_event>& goal_activated_producer;
};

#endif
