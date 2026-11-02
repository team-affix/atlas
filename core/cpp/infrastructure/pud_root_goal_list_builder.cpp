#include "infrastructure/pud_root_goal_list_builder.hpp"
#include <utility>
#include <immer/map_transient.hpp>

pud_goal_list pud_root_goal_list_builder::build(const std::vector<const expr*>& opened_goals) const {
    auto transient = immer::map<pud_body_goal_id, const expr*>{}.transient();
    pud_body_goal_id next_id = 0;
    for (const expr* goal : opened_goals) {
        transient.set(next_id, goal);
        ++next_id;
    }
    return pud_goal_list{.goals = std::move(transient).persistent(), .next_id = next_id};
}
