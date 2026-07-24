#include "infrastructure/goal_work_values.hpp"

double goal_work_values::get(const goal_lineage* gl) const {
    return values_.at(gl);
}

void goal_work_values::set(const goal_lineage* gl, double w) {
    auto [_, inserted] = values_.insert({gl, w});
    DEBUG_ASSERT(inserted);
}

void goal_work_values::erase(const goal_lineage* gl) {
    auto erased = values_.erase(gl);
    DEBUG_ASSERT(erased == 1);
}

void goal_work_values::clear_goal_work_values() {
    values_.clear();
}
