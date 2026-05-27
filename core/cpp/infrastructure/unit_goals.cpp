#include "../../hpp/infrastructure/unit_goals.hpp"

void unit_goals::push(const goal_lineage* gl) {
    queue_.push_back(gl);
}

const goal_lineage* unit_goals::pop() {
    const goal_lineage* gl = queue_.back();
    queue_.pop_back();
    return gl;
}

bool unit_goals::empty() const {
    return queue_.empty();
}

void unit_goals::clear() {
    queue_.clear();
}
