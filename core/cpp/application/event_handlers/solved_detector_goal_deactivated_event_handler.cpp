#include "../../../hpp/application/event_handlers/solved_detector_goal_deactivated_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

solved_detector_goal_deactivated_event_handler::solved_detector_goal_deactivated_event_handler() :
    solved_detector(locator::locate<i_solved_detector>()) {}

void solved_detector_goal_deactivated_event_handler::handle(const goal_deactivated_event&) {
    solved_detector.detect_solved();
}
