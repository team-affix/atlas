#ifndef DECISION_STORE_DECIDED_EVENT_HANDLER_HPP
#define DECISION_STORE_DECIDED_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/decided_event.hpp"
#include "../../domain/interfaces/i_decision_store.hpp"

struct decision_store_decided_event_handler : event_handler<decided_event> {
    decision_store_decided_event_handler();
    void handle(const decided_event&) override;
private:
    i_decision_store& decision_store;
};

#endif
