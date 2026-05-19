#include "../../hpp/infrastructure/goal_activator.hpp"

goal_activator::goal_activator(
    i_activate_goal_expr& age,
    i_get_candidate_translation_map& gctm,
    i_copier& copier)
    :
    age(age),
    gctm(gctm),
    copier(copier) {}

void goal_activator::activate(const goal_lineage* gl) {
    // 1. get the parent resolution lineage
    auto rl = gl->parent;

    // 2. get the candidate translation map
    auto& tm = gctm.get(rl);
    
    // 3. make a copy of the goal expression
    const expr* copy = copier.copy(gl->idx, tm);

    // 4. activate the goal expression
    age.activate(gl, copy);
}
