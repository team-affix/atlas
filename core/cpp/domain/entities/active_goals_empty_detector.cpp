#include "../../../hpp/domain/entities/active_goals_empty_detector.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

active_goals_empty_detector::active_goals_empty_detector() :
    active_goal_store(resolver::resolve<i_active_goal_store>()),
    active_goals_empty_producer(resolver::resolve<i_event_producer<active_goals_empty_event>>()) {}

void active_goals_empty_detector::goal_deactivated() {
    if (active_goal_store.size() == 0)
        active_goals_empty_producer.produce(active_goals_empty_event{});
}
