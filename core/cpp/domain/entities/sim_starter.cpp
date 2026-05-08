#include "../../../hpp/domain/entities/sim_starter.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

sim_starter::sim_starter() :
    trail(resolver::resolve<i_trail>()),
    initial_goal_activator(resolver::resolve<i_initial_goal_activator>()) {}

void sim_starter::start() {
    trail.push();
    initial_goal_activator.activate_initial_goals();
}
