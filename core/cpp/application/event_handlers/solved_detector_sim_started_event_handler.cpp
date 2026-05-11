#include "../../../hpp/application/event_handlers/solved_detector_sim_started_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

solved_detector_sim_started_event_handler::solved_detector_sim_started_event_handler() :
    solved_detector(locator::resolve<i_solved_detector>()) {}

void solved_detector_sim_started_event_handler::handle(const sim_started_event&) {
    solved_detector.detect_solved();
}
