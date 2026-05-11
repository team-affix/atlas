#ifndef GOAL_CANDIDATES_DEACTIVATOR_GOAL_DEACTIVATING_EVENT_HANDLER_HPP
#define GOAL_CANDIDATES_DEACTIVATOR_GOAL_DEACTIVATING_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/goal_deactivating_event.hpp"
#include "../../domain/interfaces/i_goal_candidates_deactivator.hpp"

struct goal_candidates_deactivator_goal_deactivating_event_handler : event_handler<goal_deactivating_event> {
    goal_candidates_deactivator_goal_deactivating_event_handler();
    void handle(const goal_deactivating_event&) override;
private:
    i_goal_candidates_deactivator& goal_candidates_deactivator;
};

#endif
