#ifndef FRONTIER_GOAL_ACTIVATING_EVENT_HANDLER_HPP
#define FRONTIER_GOAL_ACTIVATING_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/goal_activating_event.hpp"
#include "../../domain/interfaces/i_frontier.hpp"
#include "../../domain/interfaces/i_factory.hpp"
#include "../../domain/value_objects/goal.hpp"

struct frontier_goal_activating_event_handler : event_handler<goal_activating_event> {
    virtual ~frontier_goal_activating_event_handler() = default;
    frontier_goal_activating_event_handler();
    void handle(const goal_activating_event&) override;
private:
    i_frontier& frontier_;
    i_factory<goal, const goal_lineage*>& goal_factory_;
};

#endif