#include "../../../hpp/domain/entities/conflicted_detector.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

conflicted_detector::conflicted_detector() :
    conflicted_producer(locator::locate<i_event_producer<conflicted_event>>()) {}

void conflicted_detector::candidates_empty() {
    conflicted_producer.produce(conflicted_event{});
}

void conflicted_detector::avoidance_empty() {
    conflicted_producer.produce(conflicted_event{});
}
