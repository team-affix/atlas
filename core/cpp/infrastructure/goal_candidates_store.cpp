#include "../../hpp/infrastructure/goal_candidates_store.hpp"

void goal_candidates_store::insert(const goal_lineage* gl, candidate_set cs) {
    goal_candidates.insert({gl, std::move(cs)});
}

void goal_candidates_store::erase(const goal_lineage* gl) {
    goal_candidates.erase(gl);
}

void goal_candidates_store::clear() {
    goal_candidates.clear();
}

bool goal_candidates_store::contains(const goal_lineage* gl) const {
    return goal_candidates.contains(gl);
}

candidate_set& goal_candidates_store::at(const goal_lineage* gl) {
    return goal_candidates.at(gl);
}

const candidate_set& goal_candidates_store::at(const goal_lineage* gl) const {
    return goal_candidates.at(gl);
}
