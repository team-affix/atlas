#include "../../../hpp/application/event_handlers/conflicted_sim_stopping_bridge_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

conflicted_sim_stopping_bridge_event_handler::conflicted_sim_stopping_bridge_event_handler() :
    sim_stopping_producer(resolver::resolve<i_event_producer<sim_stopping_event>>()) {}

void conflicted_sim_stopping_bridge_event_handler::handle(const conflicted_event&) {
    sim_stopping_producer.produce(sim_stopping_event{});
}
