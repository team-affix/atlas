#include "../../../hpp/domain/entities/solved_detector.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

solved_detector::solved_detector() :
    active_goal_store(locator::locate<i_active_goal_store>()),
    solved_producer(locator::locate<i_event_producer<solved_event>>()) {}

void solved_detector::detect_solved() {
    if (active_goal_store.size() == 0)
        solved_producer.produce(solved_event{});
}
