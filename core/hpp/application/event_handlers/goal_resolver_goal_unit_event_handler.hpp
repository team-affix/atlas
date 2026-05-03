#ifndef GOAL_RESOLVER_GOAL_UNIT_EVENT_HANDLER_HPP
#define GOAL_RESOLVER_GOAL_UNIT_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/goal_unit_event.hpp"
#include "../../domain/interfaces/i_goal_resolver.hpp"

struct goal_resolver_goal_unit_event_handler : event_handler<goal_unit_event> {
    goal_resolver_goal_unit_event_handler();
    void handle(const goal_unit_event&) override;
private:
    i_goal_resolver& goal_resolver;
};

#endif
