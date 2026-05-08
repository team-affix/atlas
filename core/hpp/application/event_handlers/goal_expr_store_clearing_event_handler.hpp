#ifndef GOAL_EXPR_STORE_CLEARING_EVENT_HANDLER_HPP
#define GOAL_EXPR_STORE_CLEARING_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/goal_stores_clearing_event.hpp"
#include "../../domain/interfaces/i_goal_expr_store.hpp"

struct goal_expr_store_clearing_event_handler : event_handler<goal_stores_clearing_event> {
    goal_expr_store_clearing_event_handler();
    void handle(const goal_stores_clearing_event&) override;
private:
    i_goal_expr_store& goal_expr_store;
};

#endif
