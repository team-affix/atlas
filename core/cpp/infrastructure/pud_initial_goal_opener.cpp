#include "infrastructure/pud_initial_goal_opener.hpp"
#include <utility>
#include <immer/map_transient.hpp>

pud_goal_list pud_initial_goal_opener::open_goals(const std::vector<const expr*>& opened_goals) const {
    auto transient = immer::map<pud_body_goal_id, const expr*>{}.transient();
    pud_body_goal_id next_id = 0;
    for (const expr* goal : opened_goals) {
        transient.set(next_id, goal);
        ++next_id;
    }
    return pud_goal_list{.goals = std::move(transient).persistent(), .next_id = next_id};
}
