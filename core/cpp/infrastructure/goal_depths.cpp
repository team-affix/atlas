#include "infrastructure/goal_depths.hpp"

size_t goal_depths::get(const goal_lineage* gl) const {
    return depths_.at(gl);
}

void goal_depths::set(const goal_lineage* gl, size_t depth) {
    auto [_, inserted] = depths_.insert({gl, depth});
    DEBUG_ASSERT(inserted);
}

void goal_depths::erase(const goal_lineage* gl) {
    auto erased = depths_.erase(gl);
    DEBUG_ASSERT(erased == 1);
}

void goal_depths::clear_goal_depths() {
    depths_.clear();
}
