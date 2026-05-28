#include "../../hpp/infrastructure/initial_goal_exprs.hpp"

void initial_goal_exprs::push(const expr* e) {
    exprs_.push_back(e);
}

size_t initial_goal_exprs::count() const {
    return exprs_.size();
}

const expr* initial_goal_exprs::get(size_t idx) const {
    return exprs_.at(idx);
}
