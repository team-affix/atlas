#include "../../../hpp/application/event_handlers/goal_candidates_empty_conflicted_bridge_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

goal_candidates_empty_conflicted_bridge_event_handler::goal_candidates_empty_conflicted_bridge_event_handler() :
    conflicted_producer(resolver::resolve<i_event_producer<conflicted_event>>()) {}

void goal_candidates_empty_conflicted_bridge_event_handler::handle(const goal_candidates_empty_event&) {
    conflicted_producer.produce(conflicted_event{});
}
