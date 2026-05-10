#include "../../hpp/infrastructure/weight_frontier.hpp"

void weight_frontier::insert(const goal_lineage* gl, double w) {
    goal_weights.insert({gl, w});
}

bool weight_frontier::contains(const goal_lineage* gl) const {
    return goal_weights.contains(gl);
}

double& weight_frontier::at(const goal_lineage* gl) {
    return goal_weights.at(gl);
}

const double& weight_frontier::at(const goal_lineage* gl) const {
    return goal_weights.at(gl);
}

void weight_frontier::erase(const goal_lineage* gl) {
    goal_weights.erase(gl);
}

void weight_frontier::clear() {
    goal_weights.clear();
}
