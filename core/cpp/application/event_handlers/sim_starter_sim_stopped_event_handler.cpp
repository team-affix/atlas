#include "../../../hpp/application/event_handlers/sim_starter_sim_stopped_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

sim_starter_sim_stopped_event_handler::sim_starter_sim_stopped_event_handler() :
    sim_starter(resolver::resolve<i_sim_starter>()) {}

void sim_starter_sim_stopped_event_handler::execute(const sim_stopped_event&) {
    sim_starter.start();
}
