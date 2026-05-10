#ifndef DECISION_MEMORY_CLEARING_EVENT_HANDLER_HPP
#define DECISION_MEMORY_CLEARING_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/goal_stores_clearing_event.hpp"
#include "../../domain/interfaces/i_decision_memory.hpp"

struct decision_memory_clearing_event_handler : event_handler<goal_stores_clearing_event> {
    decision_memory_clearing_event_handler();
    void handle(const goal_stores_clearing_event&) override;
private:
    i_decision_memory& decision_memory;
};

#endif
