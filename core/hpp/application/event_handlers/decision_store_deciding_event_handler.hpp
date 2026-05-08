#ifndef DECISION_STORE_DECIDING_EVENT_HANDLER_HPP
#define DECISION_STORE_DECIDING_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/deciding_event.hpp"
#include "../../domain/interfaces/i_decision_store.hpp"

struct decision_store_deciding_event_handler : event_handler<deciding_event> {
    decision_store_deciding_event_handler();
    void handle(const deciding_event&) override;
private:
    i_decision_store& decision_store;
};

#endif
