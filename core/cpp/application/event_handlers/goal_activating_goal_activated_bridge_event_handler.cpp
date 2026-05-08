#include "../../../hpp/application/event_handlers/goal_activating_goal_activated_bridge_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

goal_activating_goal_activated_bridge_event_handler::goal_activating_goal_activated_bridge_event_handler() :
    goal_activated_producer(resolver::resolve<i_event_producer<goal_activated_event>>()) {
}

void goal_activating_goal_activated_bridge_event_handler::execute(const goal_activating_event& event) {
    goal_activated_producer.produce(goal_activated_event{event.gl});
}
