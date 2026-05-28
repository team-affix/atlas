#include "infrastructure/solution_detector.hpp"

solution_detector::solution_detector(locator& loc)
    : check_active_goals_empty(loc.locate<i_check_active_goals_empty>()) {}

bool solution_detector::detect() const {
    return check_active_goals_empty.empty();
}
