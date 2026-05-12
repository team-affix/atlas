#include "../../hpp/infrastructure/frontier.hpp"

void frontier::insert(const goal_lineage* gl, std::unique_ptr<i_goal> g) {
    goals_.emplace(gl, std::move(g));
}

bool frontier::contains(const goal_lineage* gl) const {
    return goals_.count(gl) > 0;
}

i_goal& frontier::at(const goal_lineage* gl) {
    return *goals_.at(gl);
}

const i_goal& frontier::at(const goal_lineage* gl) const {
    return *goals_.at(gl);
}

void frontier::erase(const goal_lineage* gl) {
    goals_.erase(gl);
}

void frontier::clear() {
    goals_.clear();
}
