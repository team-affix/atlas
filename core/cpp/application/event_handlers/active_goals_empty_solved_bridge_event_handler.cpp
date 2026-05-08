#include "../../../hpp/application/event_handlers/active_goals_empty_solved_bridge_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

active_goals_empty_solved_bridge_event_handler::active_goals_empty_solved_bridge_event_handler() :
    solved_producer(resolver::resolve<i_event_producer<solved_event>>()) {}

void active_goals_empty_solved_bridge_event_handler::handle(const active_goals_empty_event&) {
    solved_producer.produce(solved_event{});
}
