#include "../../../hpp/application/event_handlers/solved_sim_stopping_bridge_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

solved_sim_stopping_bridge_event_handler::solved_sim_stopping_bridge_event_handler() :
    sim_stopping_producer(resolver::resolve<i_event_producer<sim_stopping_event>>()) {}

void solved_sim_stopping_bridge_event_handler::handle(const solved_event&) {
    sim_stopping_producer.produce(sim_stopping_event{});
}
