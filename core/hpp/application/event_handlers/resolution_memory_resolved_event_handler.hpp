#ifndef RESOLUTION_MEMORY_RESOLVED_EVENT_HANDLER_HPP
#define RESOLUTION_MEMORY_RESOLVED_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/resolved_event.hpp"
#include "../../domain/interfaces/i_resolution_memory.hpp"

struct resolution_memory_resolved_event_handler : event_handler<resolved_event> {
    resolution_memory_resolved_event_handler();
    void handle(const resolved_event&) override;
private:
    i_resolution_memory& resolution_memory;
};

#endif
