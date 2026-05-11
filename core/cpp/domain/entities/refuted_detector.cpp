#include "../../../hpp/domain/entities/refuted_detector.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

refuted_detector::refuted_detector() :
    decision_memory(locator::resolve<i_decision_memory>()),
    refuted_producer(locator::resolve<i_event_producer<refuted_event>>()) {}

void refuted_detector::conflicted() {
    if (decision_memory.size() == 0)
        refuted_producer.produce(refuted_event{});
}
