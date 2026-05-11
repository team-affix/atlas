#include "../../../hpp/domain/entities/goal_candidates_empty_detector.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

goal_candidates_empty_detector::goal_candidates_empty_detector() :
    candidates_frontier(locator::resolve<i_candidates_frontier>()),
    goal_candidates_empty_event_producer(locator::resolve<i_event_producer<goal_candidates_empty_event>>()) {
}

void goal_candidates_empty_detector::candidates_changed(const goal_lineage* gl) {
    if (candidates_frontier.at(gl).candidates.empty())
        goal_candidates_empty_event_producer.produce(goal_candidates_empty_event{gl});
}
