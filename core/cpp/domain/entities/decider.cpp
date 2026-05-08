#include "../../../hpp/domain/entities/decider.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

decider::decider() :
    decision_generator(resolver::resolve<i_decision_generator>()),
    deciding_producer(resolver::resolve<i_event_producer<deciding_event>>()),
    decided_producer(resolver::resolve<i_event_producer<decided_event>>()) {
}

void decider::decide() const {
    auto rl = decision_generator.generate();
    deciding_producer.produce(deciding_event{rl});
    decided_producer.produce(decided_event{rl});
}
