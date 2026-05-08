#include "../../../hpp/application/event_handlers/sim_starting_sim_starter_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

sim_starting_sim_starter_event_handler::sim_starting_sim_starter_event_handler() :
    sim_starter(resolver::resolve<i_sim_starter>()) {}

void sim_starting_sim_starter_event_handler::handle(const sim_starting_event&) {
    sim_starter.start();
}
