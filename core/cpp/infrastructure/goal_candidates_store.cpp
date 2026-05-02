#include "../../hpp/infrastructure/goal_candidates_store.hpp"

void goal_candidates_store::insert(const goal_lineage* gl, const candidate_set& cs) {
    goal_candidates.insert({gl, cs});
}

void goal_candidates_store::erase(const goal_lineage* gl) {
    goal_candidates.erase(gl);
}

void goal_candidates_store::eliminate(const goal_lineage* gl, size_t c) {
    goal_candidates.at(gl).candidates.erase(c);
}

void goal_candidates_store::clear() {
    goal_candidates.clear();
}

const candidate_set& goal_candidates_store::at(const goal_lineage* gl) const {
    return goal_candidates.at(gl);
}
