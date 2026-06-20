#include "infrastructure/unit_goals.hpp"

void unit_goals::push(const goal_lineage* gl) {
    queue_.push_back(gl);
}

std::optional<const goal_lineage*> unit_goals::pop() {
    if (queue_.empty()) return std::nullopt;
    const goal_lineage* gl = queue_.back();
    queue_.pop_back();
    return gl;
}

void unit_goals::clear() {
    queue_.clear();
}
