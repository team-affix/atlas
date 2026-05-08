#ifndef ACTIVE_GOAL_STORE_CLEARING_EVENT_HANDLER_HPP
#define ACTIVE_GOAL_STORE_CLEARING_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/goal_stores_clearing_event.hpp"
#include "../../domain/interfaces/i_active_goal_store.hpp"

struct active_goal_store_clearing_event_handler : event_handler<goal_stores_clearing_event> {
    active_goal_store_clearing_event_handler();
    void handle(const goal_stores_clearing_event&) override;
private:
    i_active_goal_store& active_goal_store;
};

#endif
