#include "../../../hpp/application/event_handlers/goal_stores_clearing_cleared_bridge_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

goal_stores_clearing_cleared_bridge_event_handler::goal_stores_clearing_cleared_bridge_event_handler() :
    goal_stores_cleared_producer(resolver::resolve<i_event_producer<goal_stores_cleared_event>>()) {}

void goal_stores_clearing_cleared_bridge_event_handler::execute(const goal_stores_clearing_event&) {
    goal_stores_cleared_producer.produce(goal_stores_cleared_event{});
}
