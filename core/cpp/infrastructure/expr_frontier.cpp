#include "../../hpp/infrastructure/expr_frontier.hpp"

void expr_frontier::insert(const goal_lineage* gl, const expr* e) {
    goal_exprs.insert({gl, e});
}

bool expr_frontier::contains(const goal_lineage* gl) const {
    return goal_exprs.contains(gl);
}

const expr*& expr_frontier::at(const goal_lineage* gl) {
    return goal_exprs.at(gl);
}

const expr* const& expr_frontier::at(const goal_lineage* gl) const {
    return goal_exprs.at(gl);
}

void expr_frontier::erase(const goal_lineage* gl) {
    goal_exprs.erase(gl);
}

void expr_frontier::clear() {
    goal_exprs.clear();
}
