#include "../../../hpp/domain/entities/refuted_detector.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

refuted_detector::refuted_detector() :
    decision_memory(locator::locate<i_decision_memory>()),
    refuted_producer(locator::locate<i_event_producer<refuted_event>>()) {}

void refuted_detector::conflicted() {
    if (decision_memory.size() == 0)
        refuted_producer.produce(refuted_event{});
}
