#include "../../hpp/infrastructure/goal_weight_store.hpp"

void goal_weight_store::insert(const goal_lineage* gl, double w) {
    goal_weights.insert({gl, w});
}

void goal_weight_store::erase(const goal_lineage* gl) {
    goal_weights.erase(gl);
}

void goal_weight_store::clear() {
    goal_weights.clear();
}

double& goal_weight_store::at(const goal_lineage* gl) {
    return goal_weights.at(gl);
}

const double& goal_weight_store::at(const goal_lineage* gl) const {
    return goal_weights.at(gl);
}
