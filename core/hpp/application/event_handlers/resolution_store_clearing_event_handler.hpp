#ifndef RESOLUTION_STORE_CLEARING_EVENT_HANDLER_HPP
#define RESOLUTION_STORE_CLEARING_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/goal_stores_clearing_event.hpp"
#include "../../domain/interfaces/i_resolution_store.hpp"

struct resolution_store_clearing_event_handler : event_handler<goal_stores_clearing_event> {
    resolution_store_clearing_event_handler();
    void handle(const goal_stores_clearing_event&) override;
private:
    i_resolution_store& resolution_store;
};

#endif
