#ifndef CONFLICTED_DETECTOR_GOAL_CANDIDATES_EMPTY_EVENT_HANDLER_HPP
#define CONFLICTED_DETECTOR_GOAL_CANDIDATES_EMPTY_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/goal_candidates_empty_event.hpp"
#include "../../domain/interfaces/i_conflicted_detector.hpp"

struct conflicted_detector_goal_candidates_empty_event_handler : event_handler<goal_candidates_empty_event> {
    conflicted_detector_goal_candidates_empty_event_handler();
    void handle(const goal_candidates_empty_event&) override;
private:
    i_conflicted_detector& conflicted_detector;
};

#endif
