#include "../../hpp/infrastructure/solution_detector.hpp"

solution_detector::solution_detector(i_active_goals& ag) : ag(ag) {}

bool solution_detector::detect() const {
    return ag.empty();
}
