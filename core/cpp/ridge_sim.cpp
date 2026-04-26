#include "../hpp/ridge_sim.hpp"

ridge_sim::ridge_sim() :
    sim(),
    dec() {
}

const resolution_lineage* ridge_sim::decide_one() {
    auto [chosen_goal, chosen_candidate] = dec();
    return lp.resolution(chosen_goal, chosen_candidate);
}

void ridge_sim::on_resolve(const resolution_lineage*) {}
