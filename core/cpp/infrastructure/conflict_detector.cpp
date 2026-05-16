#include "../../hpp/infrastructure/conflict_detector.hpp"

bool conflict_detector::detect(const goal& goal) {
    return goal.candidates.empty();
}
