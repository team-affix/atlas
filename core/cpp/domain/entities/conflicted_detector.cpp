#include "../../../hpp/domain/entities/conflicted_detector.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

conflicted_detector::conflicted_detector() :
    conflicted_producer(resolver::resolve<i_event_producer<conflicted_event>>()) {}

void conflicted_detector::candidates_empty() {
    conflicted_producer.produce(conflicted_event{});
}

void conflicted_detector::avoidance_empty() {
    conflicted_producer.produce(conflicted_event{});
}
