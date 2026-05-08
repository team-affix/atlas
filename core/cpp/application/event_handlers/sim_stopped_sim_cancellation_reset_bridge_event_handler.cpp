#include "../../../hpp/application/event_handlers/sim_stopped_sim_cancellation_reset_bridge_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

sim_stopped_sim_cancellation_reset_bridge_event_handler::sim_stopped_sim_cancellation_reset_bridge_event_handler() :
    sim_cancellation_reset_producer(resolver::resolve<i_event_producer<sim_cancellation_reset_event>>()) {}

void sim_stopped_sim_cancellation_reset_bridge_event_handler::handle(const sim_stopped_event&) {
    sim_cancellation_reset_producer.produce(sim_cancellation_reset_event{});
}
