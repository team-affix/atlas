#include "../../../hpp/application/event_handlers/conflicted_detector_avoidance_empty_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

conflicted_detector_avoidance_empty_event_handler::conflicted_detector_avoidance_empty_event_handler() :
    conflicted_detector(resolver::resolve<i_conflicted_detector>()) {}

void conflicted_detector_avoidance_empty_event_handler::execute(const avoidance_empty_event&) {
    conflicted_detector.avoidance_empty();
}
