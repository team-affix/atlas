#include "../../../hpp/domain/entities/sim_starter.hpp"
#include "../../../hpp/domain/events/fixpoint_reached_event.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

sim_starter::sim_starter() :
    trail(resolver::resolve<i_trail>()),
    goal_resolver(resolver::resolve<i_goal_resolver>()),
    sim_started_producer(resolver::resolve<i_event_producer<sim_started_event>>()),
    fixpoint_reached_producer(resolver::resolve<i_event_producer<fixpoint_reached_event>>()) {}

void sim_starter::start() {
    trail.push();
    goal_resolver.init_resolve(nullptr);
}

void sim_starter::complete_start() {
    sim_started_producer.produce(sim_started_event{});
    fixpoint_reached_producer.produce(fixpoint_reached_event{});
}
