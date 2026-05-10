#include "../../hpp/infrastructure/goal_candidates_store.hpp"

void goal_candidates_store::insert(const goal_lineage* gl, candidate_set cs) {
    goal_candidates.insert({gl, std::move(cs)});
}

void goal_candidates_store::erase(const goal_lineage* gl) {
    goal_candidates.erase(gl);
}

void goal_candidates_store::eliminate(const resolution_lineage* rl) {
    goal_candidates.at(rl->parent).candidates.erase(rl->idx);
}

void goal_candidates_store::clear() {
    goal_candidates.clear();
}

candidate_set& goal_candidates_store::get(const goal_lineage* gl) {
    return goal_candidates.at(gl);
}
