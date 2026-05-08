#ifndef GOAL_CANDIDATES_STORE_CLEARING_EVENT_HANDLER_HPP
#define GOAL_CANDIDATES_STORE_CLEARING_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/goal_stores_clearing_event.hpp"
#include "../../domain/interfaces/i_goal_candidates_store.hpp"

struct goal_candidates_store_clearing_event_handler : event_handler<goal_stores_clearing_event> {
    goal_candidates_store_clearing_event_handler();
    void handle(const goal_stores_clearing_event&) override;
private:
    i_goal_candidates_store& goal_candidates_store;
};

#endif
