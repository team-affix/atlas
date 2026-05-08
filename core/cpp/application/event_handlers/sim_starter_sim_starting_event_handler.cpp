#include "../../../hpp/application/event_handlers/sim_starter_sim_starting_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

sim_starter_sim_starting_event_handler::sim_starter_sim_starting_event_handler() :
    sim_starter(resolver::resolve<i_sim_starter>()) {}

void sim_starter_sim_starting_event_handler::handle(const sim_starting_event&) {
    sim_starter.start();
}
