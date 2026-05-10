#include "../../hpp/infrastructure/candidates_frontier.hpp"

void candidates_frontier::insert(const goal_lineage* gl, candidate_set cs) {
    goal_candidates.insert({gl, std::move(cs)});
}

bool candidates_frontier::contains(const goal_lineage* gl) const {
    return goal_candidates.contains(gl);
}

candidate_set& candidates_frontier::at(const goal_lineage* gl) {
    return goal_candidates.at(gl);
}

const candidate_set& candidates_frontier::at(const goal_lineage* gl) const {
    return goal_candidates.at(gl);
}

void candidates_frontier::erase(const goal_lineage* gl) {
    goal_candidates.erase(gl);
}

void candidates_frontier::clear() {
    goal_candidates.clear();
}
