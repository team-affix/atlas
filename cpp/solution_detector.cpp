#include "../hpp/solution_detector.hpp"

solution_detector::solution_detector(
    const a01_goal_store& gs
)
    : gs(gs)
{}

bool solution_detector::operator()() {
    return gs.empty();
}
