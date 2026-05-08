#ifndef GOAL_DEACTIVATING_GOAL_DEACTIVATED_BRIDGE_EVENT_HANDLER_HPP
#define GOAL_DEACTIVATING_GOAL_DEACTIVATED_BRIDGE_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/goal_deactivating_event.hpp"
#include "../../domain/events/goal_deactivated_event.hpp"
#include "../../domain/interfaces/i_event_producer.hpp"

struct goal_deactivating_goal_deactivated_bridge_event_handler : event_handler<goal_deactivating_event> {
    goal_deactivating_goal_deactivated_bridge_event_handler();
    void handle(const goal_deactivating_event& event) override;
private:
    i_event_producer<goal_deactivated_event>& goal_deactivated_producer;
};

#endif
