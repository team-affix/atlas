#ifndef INACTIVE_GOAL_STORE_CLEARING_EVENT_HANDLER_HPP
#define INACTIVE_GOAL_STORE_CLEARING_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/goal_stores_clearing_event.hpp"
#include "../../domain/interfaces/i_inactive_goal_store.hpp"

struct inactive_goal_store_clearing_event_handler : event_handler<goal_stores_clearing_event> {
    inactive_goal_store_clearing_event_handler();
    void handle(const goal_stores_clearing_event&) override;
private:
    i_inactive_goal_store& inactive_goal_store;
};

#endif
