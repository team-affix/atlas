#include "infrastructure/goal_exprs.hpp"
#include "debug_assert.hpp"

const expr* goal_exprs::get(const goal_lineage* gl) const {
    return exprs_.at(gl);
}

void goal_exprs::set(const goal_lineage* gl, const expr* e) {
    auto [_, inserted] = exprs_.insert({gl, e});
    DEBUG_ASSERT(inserted);
}

void goal_exprs::unset(const goal_lineage* gl) {
    auto erased = exprs_.erase(gl);
    DEBUG_ASSERT(erased == 1);
}

void goal_exprs::clear_goal_exprs() {
    exprs_.clear();
}
