#include "../../../hpp/application/event_handlers/refuted_detector_conflicted_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

refuted_detector_conflicted_event_handler::refuted_detector_conflicted_event_handler() :
    detector(locator::resolve<i_refuted_detector>()) {}

void refuted_detector_conflicted_event_handler::handle(const conflicted_event&) {
    detector.conflicted();
}
