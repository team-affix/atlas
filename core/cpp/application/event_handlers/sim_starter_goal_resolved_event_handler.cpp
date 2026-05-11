#include "../../../hpp/application/event_handlers/sim_starter_goal_resolved_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

sim_starter_goal_resolved_event_handler::sim_starter_goal_resolved_event_handler() :
    sim_starter(resolver::resolve<i_sim_starter>()) {}

void sim_starter_goal_resolved_event_handler::handle(const goal_resolved_event& e) {
    if (e.rl == nullptr)
        sim_starter.complete_start();
}
