#include "../../../hpp/domain/entities/solved_detector.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

solved_detector::solved_detector() :
    f(locator::locate<i_frontier>()),
    solved_producer(locator::locate<i_event_producer<solved_event>>()) {}

void solved_detector::detect_solved() {
    if (f.size() == 0)
        solved_producer.produce(solved_event{});
}
