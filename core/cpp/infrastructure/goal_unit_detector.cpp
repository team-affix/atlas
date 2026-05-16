#include "../../../hpp/domain/entities/goal_unit_detector.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

goal_unit_detector::goal_unit_detector() :
    f(locator::locate<i_frontier>()),
    goal_unit_event_producer(locator::locate<i_event_producer<goal_unit_event>>()) {
}

void goal_unit_detector::candidates_changed(const goal_lineage* gl) {
    if (f.at(gl)->candidates.size() == 1)
        goal_unit_event_producer.produce(goal_unit_event{gl});
}
