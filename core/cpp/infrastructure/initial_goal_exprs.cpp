#include "../../hpp/infrastructure/initial_goal_exprs.hpp"

void initial_goal_exprs::push(const expr* e) {
    exprs_.push_back(e);
}

const expr* initial_goal_exprs::at(size_t i) const {
    return exprs_.at(i);
}

size_t initial_goal_exprs::size() const {
    return exprs_.size();
}
