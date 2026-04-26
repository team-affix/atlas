#include <stdexcept>
#include "../hpp/domain/data_structures/goal_expander.hpp"

goal_expander::goal_expander(
    const expr* const& goal,
    const rule& r,
    bind_map& bm,
    copier& cp
) :
    cp(cp),
    subgoal_index(0) {
    // copy the head of the rule
    const expr* copied_head = cp(r.head, translation_map);

    // unify the goal with the head of the rule
    if (!bm.unify(goal, copied_head))
        throw std::runtime_error("Failed to unify the goal with the head of the rule");

    // save the original body
    rule_body = r.body;
}

const expr* goal_expander::operator()() {
    // copy the member of the rule body at the subgoal index
    return cp(rule_body.at(subgoal_index++), translation_map);
}
