#include "../../../hpp/domain/entities/sim_starter.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

sim_starter::sim_starter() :
    trail(resolver::resolve<i_trail>()),
    initial_goal_activator(resolver::resolve<i_initial_goal_activator>()),
    sim_started_producer(resolver::resolve<i_event_producer<sim_started_event>>()),
    no_more_unit_goals_producer(resolver::resolve<i_event_producer<no_more_unit_goals_event>>()) {}

void sim_starter::start() {
    trail.push();
    initial_goal_activator.activate_initial_goals();
}

void sim_starter::complete_start() {
    sim_started_producer.produce(sim_started_event{});
    no_more_unit_goals_producer.produce(no_more_unit_goals_event{});
}
