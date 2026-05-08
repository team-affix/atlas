#include "../../../hpp/domain/entities/sim_restarter.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

sim_restarter::sim_restarter() :
    trail(resolver::resolve<i_trail>()),
    decision_store(resolver::resolve<i_decision_store>()),
    c(resolver::resolve<i_cdcl>()),
    initial_goal_activator(resolver::resolve<i_initial_goal_activator>()),
    goal_stores_clearing_producer(resolver::resolve<i_event_producer<goal_stores_clearing_event>>()) {}

void sim_restarter::begin_restart() {
    trail.pop();
    pending_lemma = decision_store.derive_lemma();
    goal_stores_clearing_producer.produce(goal_stores_clearing_event{});
}

void sim_restarter::complete_restart() {
    c.learn(*pending_lemma);
    pending_lemma.reset();
    trail.push();
    initial_goal_activator.activate_initial_goals();
}
