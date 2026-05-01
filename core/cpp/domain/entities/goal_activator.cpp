#include "../../../hpp/domain/entities/goal_activator.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

goal_activator::goal_activator() :
    goal_activating_producer(resolver::resolve<i_event_producer<goal_activating_event>>()),
    goal_activated_producer(resolver::resolve<i_event_producer<goal_activated_event>>()) {
}

void goal_activator::activate(const goal_lineage* gl) {
    goal_activating_producer.produce(goal_activating_event{gl});
    goal_activated_producer.produce(goal_activated_event{gl});
}
