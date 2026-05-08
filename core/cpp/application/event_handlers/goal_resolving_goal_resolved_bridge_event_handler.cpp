#include "../../../hpp/application/event_handlers/goal_resolving_goal_resolved_bridge_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

goal_resolving_goal_resolved_bridge_event_handler::goal_resolving_goal_resolved_bridge_event_handler() :
    goal_resolved_producer(resolver::resolve<i_event_producer<goal_resolved_event>>()) {
}

void goal_resolving_goal_resolved_bridge_event_handler::handle(const goal_resolving_event& event) {
    goal_resolved_producer.produce(goal_resolved_event{event.rl});
}
