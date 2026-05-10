#ifndef CANDIDATES_FRONTIER_CLEARING_EVENT_HANDLER_HPP
#define CANDIDATES_FRONTIER_CLEARING_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/goal_stores_clearing_event.hpp"
#include "../../domain/interfaces/i_candidates_frontier.hpp"

struct candidates_frontier_clearing_event_handler : event_handler<goal_stores_clearing_event> {
    candidates_frontier_clearing_event_handler();
    void handle(const goal_stores_clearing_event&) override;
private:
    i_candidates_frontier& cf;
};

#endif
