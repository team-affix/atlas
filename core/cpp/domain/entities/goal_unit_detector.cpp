#include "../../../hpp/domain/entities/goal_unit_detector.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

goal_unit_detector::goal_unit_detector() :
    candidates_frontier(resolver::resolve<i_candidates_frontier>()),
    goal_unit_event_producer(resolver::resolve<i_event_producer<goal_unit_event>>()) {
}

void goal_unit_detector::candidates_changed(const goal_lineage* gl) {
    if (candidates_frontier.at(gl).candidates.size() == 1)
        goal_unit_event_producer.produce(goal_unit_event{gl});
}
