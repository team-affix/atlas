#include "infrastructure/goal_weights.hpp"
#include "debug_assert.hpp"

double goal_weights::get(const goal_lineage* gl) const {
    return weights_.at(gl);
}

void goal_weights::set(const goal_lineage* gl, double w) {
    auto [_, inserted] = weights_.insert({gl, w});
    DEBUG_ASSERT(inserted);
}

void goal_weights::erase(const goal_lineage* gl) {
    auto erased = weights_.erase(gl);
    DEBUG_ASSERT(erased == 1);
}

void goal_weights::clear_goal_weights() {
    weights_.clear();
}
