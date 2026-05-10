#ifndef RESOLUTION_MEMORY_GOAL_RESOLVED_EVENT_HANDLER_HPP
#define RESOLUTION_MEMORY_GOAL_RESOLVED_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/goal_resolved_event.hpp"
#include "../../domain/interfaces/i_resolution_memory.hpp"

struct resolution_memory_goal_resolved_event_handler : event_handler<goal_resolved_event> {
    resolution_memory_goal_resolved_event_handler();
    void handle(const goal_resolved_event&) override;
private:
    i_resolution_memory& resolution_memory;
};

#endif
