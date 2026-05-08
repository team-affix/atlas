#include "../../../hpp/application/event_handlers/sim_stopped_sim_starting_bridge_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

sim_stopped_sim_starting_bridge_event_handler::sim_stopped_sim_starting_bridge_event_handler() :
    sim_starting_producer(resolver::resolve<i_event_producer<sim_starting_event>>()) {}

void sim_stopped_sim_starting_bridge_event_handler::handle(const sim_stopped_event&) {
    sim_starting_producer.produce(sim_starting_event{});
}
