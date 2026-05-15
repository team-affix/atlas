#ifndef FRONTIER_GOAL_DEACTIVATING_EVENT_HANDLER_HPP
#define FRONTIER_GOAL_DEACTIVATING_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/goal_deactivating_event.hpp"
#include "../../domain/interfaces/i_frontier.hpp"

struct frontier_goal_deactivating_event_handler : event_handler<goal_deactivating_event> {
    virtual ~frontier_goal_deactivating_event_handler() = default;
    frontier_goal_deactivating_event_handler();
    void handle(const goal_deactivating_event&) override;
private:
    i_frontier& frontier_;
};

#endif