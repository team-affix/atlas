#ifndef FRONTIER_RESOLVING_EVENT_HANDLER_HPP
#define FRONTIER_RESOLVING_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/resolving_event.hpp"
#include "../../domain/interfaces/i_frontier.hpp"

struct frontier_resolving_event_handler : event_handler<resolving_event> {
    virtual ~frontier_resolving_event_handler() = default;
    frontier_resolving_event_handler();
    void handle(const resolving_event&) override;
private:
    i_frontier& frontier_;
};

#endif
