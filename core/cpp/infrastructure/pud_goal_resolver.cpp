#include "infrastructure/pud_goal_resolver.hpp"
#include <utility>
#include <immer/map_transient.hpp>

pud_goal_list pud_goal_resolver::resolve_goal(
    const pud_goal_list& goals,
    pud_body_goal_id closed_goal,
    const std::vector<const expr*>& added_goals) const {
    auto transient = goals.goals.transient();
    transient.erase(closed_goal);
    pud_body_goal_id next_id = goals.next_id;
    for (const expr* goal : added_goals) {
        transient.set(next_id, goal);
        next_id = next_id + 1;
    }
    return pud_goal_list{.goals = std::move(transient).persistent(), .next_id = next_id};
}
