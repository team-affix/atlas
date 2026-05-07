#include "../../../hpp/domain/entities/initial_goal_activator.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

initial_goal_activator::initial_goal_activator(size_t initial_goal_count) :
    lineage_pool(resolver::resolve<i_lineage_pool>()),
    initial_goal_activating_event_producer(resolver::resolve<i_event_producer<initial_goal_activating_event>>()),
    initial_goal_count(initial_goal_count) {
}

void initial_goal_activator::activate_initial_goals() {
    for (size_t i = 0; i < initial_goal_count; ++i)
        initial_goal_activating_event_producer.produce(initial_goal_activating_event{lineage_pool.goal(nullptr, i)});
}
