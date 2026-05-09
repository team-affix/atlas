#include "../../../hpp/application/event_handlers/solved_sim_termination_condition_reached_bridge_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

solved_sim_termination_condition_reached_bridge_event_handler::solved_sim_termination_condition_reached_bridge_event_handler() :
    producer(resolver::resolve<i_event_producer<sim_termination_condition_reached_event>>()) {}

void solved_sim_termination_condition_reached_bridge_event_handler::handle(const solved_event&) {
    producer.produce(sim_termination_condition_reached_event{});
}
