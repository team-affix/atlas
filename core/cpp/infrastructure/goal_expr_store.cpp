#include "../../hpp/infrastructure/goal_expr_store.hpp"

void goal_expr_store::insert(const goal_lineage* gl, const expr* e) {
    goal_exprs.insert({gl, e});
}

void goal_expr_store::erase(const goal_lineage* gl) {
    goal_exprs.erase(gl);
}

void goal_expr_store::clear() {
    goal_exprs.clear();
}

const expr*& goal_expr_store::get(const goal_lineage* gl) {
    return goal_exprs.at(gl);
}
