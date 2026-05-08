#include "../../../hpp/application/event_handlers/initial_goal_activating_goal_activated_bridge_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

initial_goal_activating_goal_activated_bridge_event_handler::initial_goal_activating_goal_activated_bridge_event_handler() :
    goal_activated_producer(resolver::resolve<i_event_producer<goal_activated_event>>()) {
}

void initial_goal_activating_goal_activated_bridge_event_handler::execute(const initial_goal_activating_event& event) {
    goal_activated_producer.produce(goal_activated_event{event.gl});
}
