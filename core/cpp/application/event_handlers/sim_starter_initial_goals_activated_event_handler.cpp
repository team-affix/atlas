#include "../../../hpp/application/event_handlers/sim_starter_initial_goals_activated_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

sim_starter_initial_goals_activated_event_handler::sim_starter_initial_goals_activated_event_handler() :
    sim_starter(resolver::resolve<i_sim_starter>()) {}

void sim_starter_initial_goals_activated_event_handler::handle(const initial_goals_activated_event&) {
    sim_starter.complete_start();
}
