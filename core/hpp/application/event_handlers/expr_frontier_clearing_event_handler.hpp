#ifndef EXPR_FRONTIER_CLEARING_EVENT_HANDLER_HPP
#define EXPR_FRONTIER_CLEARING_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/goal_stores_clearing_event.hpp"
#include "../../domain/interfaces/i_expr_frontier.hpp"

struct expr_frontier_clearing_event_handler : event_handler<goal_stores_clearing_event> {
    expr_frontier_clearing_event_handler();
    void handle(const goal_stores_clearing_event&) override;
private:
    i_expr_frontier& ef;
};

#endif
