#ifndef GOAL_RESOLVER_DECIDED_EVENT_HANDLER_HPP
#define GOAL_RESOLVER_DECIDED_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/decided_event.hpp"
#include "../../domain/interfaces/i_goal_resolver.hpp"

struct goal_resolver_decided_event_handler : event_handler<decided_event> {
    goal_resolver_decided_event_handler();
    void handle(const decided_event&) override;
private:
    i_goal_resolver& goal_resolver;
};

#endif
