#ifndef GOAL_CANDIDATES_EMPTY_CONFLICTED_BRIDGE_EVENT_HANDLER_HPP
#define GOAL_CANDIDATES_EMPTY_CONFLICTED_BRIDGE_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/goal_candidates_empty_event.hpp"
#include "../../domain/events/conflicted_event.hpp"
#include "../../domain/interfaces/i_event_producer.hpp"

struct goal_candidates_empty_conflicted_bridge_event_handler : event_handler<goal_candidates_empty_event> {
    goal_candidates_empty_conflicted_bridge_event_handler();
    void handle(const goal_candidates_empty_event&) override;
private:
    i_event_producer<conflicted_event>& conflicted_producer;
};

#endif
