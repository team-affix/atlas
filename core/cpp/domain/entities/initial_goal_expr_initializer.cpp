#include "../../../hpp/domain/entities/initial_goal_expr_initializer.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

initial_goal_expr_initializer::initial_goal_expr_initializer(const std::vector<const expr*>& initial_exprs) :
    ges(resolver::resolve<i_goal_expr_store>()),
    initial_exprs(initial_exprs) {
}

void initial_goal_expr_initializer::initialize(const goal_lineage* gl) {
    ges.insert(gl, initial_exprs.at(gl->idx));
}
