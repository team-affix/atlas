#include <stdexcept>
#include "../hpp/goal_expander.hpp"
#include "../hpp/locator.hpp"
#include "../hpp/bind_map.hpp"
#include "../hpp/copier.hpp"

goal_expander::goal_expander(
    const expr* const& goal,
    const rule& r) :
    cp(locator::locate<copier>(locator_keys::inst_copier)), subgoal_index(0) {

    bind_map& bm = locator::locate<bind_map>(locator_keys::inst_bind_map);

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
