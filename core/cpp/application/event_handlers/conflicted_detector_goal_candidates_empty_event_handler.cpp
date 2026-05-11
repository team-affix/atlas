#include "../../../hpp/application/event_handlers/conflicted_detector_goal_candidates_empty_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

conflicted_detector_goal_candidates_empty_event_handler::conflicted_detector_goal_candidates_empty_event_handler() :
    conflicted_detector(locator::locate<i_conflicted_detector>()) {}

void conflicted_detector_goal_candidates_empty_event_handler::handle(const goal_candidates_empty_event&) {
    conflicted_detector.candidates_empty();
}
