#include "../../../hpp/domain/entities/sim_stopper.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

sim_stopper::sim_stopper() :
    trail(locator::locate<i_trail>()),
    decision_memory(locator::locate<i_decision_memory>()),
    c(locator::locate<i_cdcl>()),
    goal_stores_clearing_producer(locator::locate<i_event_producer<goal_stores_clearing_event>>()),
    goal_stores_cleared_producer(locator::locate<i_event_producer<goal_stores_cleared_event>>()),
    sim_stopped_producer(locator::locate<i_event_producer<sim_stopped_event>>()) {
}

void sim_stopper::init_stop() {
    trail.pop();
    pending_lemma = decision_memory.derive_lemma();
    goal_stores_clearing_producer.produce(goal_stores_clearing_event{});
    goal_stores_cleared_producer.produce(goal_stores_cleared_event{});
}

void sim_stopper::finish_stop() {
    c.learn(*pending_lemma);
    pending_lemma.reset();
    sim_stopped_producer.produce(sim_stopped_event{});
}
