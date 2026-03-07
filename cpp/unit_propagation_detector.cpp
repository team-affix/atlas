#include "../hpp/unit_propagation_detector.hpp"

unit_propagation_detector::unit_propagation_detector(
    const a01_candidate_store& cs
)
    : cs(cs)
{}

bool unit_propagation_detector::operator()(const goal_lineage* gl) {
    return cs.count(gl) == 1;
}
