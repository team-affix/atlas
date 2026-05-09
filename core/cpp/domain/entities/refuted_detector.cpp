#include "../../../hpp/domain/entities/refuted_detector.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

refuted_detector::refuted_detector() :
    decision_store(resolver::resolve<i_decision_store>()),
    refuted_producer(resolver::resolve<i_event_producer<refuted_event>>()) {}

void refuted_detector::conflicted() {
    if (decision_store.size() == 0)
        refuted_producer.produce(refuted_event{});
}
