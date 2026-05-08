#include "../../../hpp/application/event_handlers/goal_deactivating_goal_deactivated_bridge_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

goal_deactivating_goal_deactivated_bridge_event_handler::goal_deactivating_goal_deactivated_bridge_event_handler() :
    goal_deactivated_producer(resolver::resolve<i_event_producer<goal_deactivated_event>>()) {
}

void goal_deactivating_goal_deactivated_bridge_event_handler::handle(const goal_deactivating_event& event) {
    goal_deactivated_producer.produce(goal_deactivated_event{event.gl});
}
