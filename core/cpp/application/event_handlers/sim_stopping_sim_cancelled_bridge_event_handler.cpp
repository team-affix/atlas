#include "../../../hpp/application/event_handlers/sim_stopping_sim_cancelled_bridge_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

sim_stopping_sim_cancelled_bridge_event_handler::sim_stopping_sim_cancelled_bridge_event_handler() :
    sim_cancelled_producer(resolver::resolve<i_event_producer<sim_cancelled_event>>()) {}

void sim_stopping_sim_cancelled_bridge_event_handler::handle(const sim_stopping_event&) {
    sim_cancelled_producer.produce(sim_cancelled_event{});
}
