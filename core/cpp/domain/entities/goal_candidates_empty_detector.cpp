#include "../../../hpp/domain/entities/goal_candidates_empty_detector.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

goal_candidates_empty_detector::goal_candidates_empty_detector() :
    f(locator::locate<i_frontier>()),
    goal_candidates_empty_event_producer(locator::locate<i_event_producer<goal_candidates_empty_event>>()) {
}

void goal_candidates_empty_detector::candidates_changed(const goal_lineage* gl) {
    if (f.at(gl)->candidates.empty())
        goal_candidates_empty_event_producer.produce(goal_candidates_empty_event{gl});
}
