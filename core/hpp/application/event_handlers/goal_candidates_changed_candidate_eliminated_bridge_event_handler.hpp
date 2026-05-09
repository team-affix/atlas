#ifndef GOAL_CANDIDATES_CHANGED_CANDIDATE_ELIMINATED_BRIDGE_EVENT_HANDLER_HPP
#define GOAL_CANDIDATES_CHANGED_CANDIDATE_ELIMINATED_BRIDGE_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/candidate_eliminated_event.hpp"
#include "../../domain/events/goal_candidates_changed_event.hpp"
#include "../../domain/interfaces/i_event_producer.hpp"

struct goal_candidates_changed_candidate_eliminated_bridge_event_handler : event_handler<candidate_eliminated_event> {
    goal_candidates_changed_candidate_eliminated_bridge_event_handler();
    void handle(const candidate_eliminated_event&) override;
private:
    i_event_producer<goal_candidates_changed_event>& producer;
};

#endif
