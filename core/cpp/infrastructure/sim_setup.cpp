#include "../../hpp/infrastructure/sim_setup.hpp"

sim_setup::sim_setup(
    i_trail& trail)
    : trail(trail) {
}

void sim_setup::setup() {
    // 1. setup the trail
    trail.setup();
}
