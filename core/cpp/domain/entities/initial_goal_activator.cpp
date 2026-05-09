#include "../../../hpp/domain/entities/initial_goal_activator.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

initial_goal_activator::initial_goal_activator(size_t initial_goal_count) :
    lineage_pool(resolver::resolve<i_lineage_pool>()),
    initial_goal_activating_producer(resolver::resolve<i_event_producer<initial_goal_activating_event>>()),
    goal_activated_producer(resolver::resolve<i_event_producer<goal_activated_event>>()),
    initial_goals_activated_producer(resolver::resolve<i_event_producer<initial_goals_activated_event>>()),
    initial_goal_count(initial_goal_count) {
}

void initial_goal_activator::activate_initial_goals() {
    for (size_t i = 0; i < initial_goal_count; ++i) {
        const goal_lineage* gl = lineage_pool.goal(nullptr, i);
        initial_goal_activating_producer.produce(initial_goal_activating_event{gl});
        goal_activated_producer.produce(goal_activated_event{gl});
    }
    initial_goals_activated_producer.produce(initial_goals_activated_event{});
}
