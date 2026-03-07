#include "../hpp/a01_cdcl_elimination_detector.hpp"

a01_cdcl_elimination_detector::a01_cdcl_elimination_detector(
    const a01_avoidance_store& as,
    lineage_pool& lp
)
    : as(as), lp(lp)
{}

bool a01_cdcl_elimination_detector::operator()(const goal_lineage* gl, size_t i) {
    // Construct the resolution lineage
    const resolution_lineage* rl = lp.resolution(gl, i);

    // Check if the resolution lineage exists by itself in the avoidance store
    return as.count({rl}) > 0;
}
