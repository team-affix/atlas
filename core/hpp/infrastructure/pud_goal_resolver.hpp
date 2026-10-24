#ifndef PUD_GOAL_RESOLVER_HPP
#define PUD_GOAL_RESOLVER_HPP

#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_goal_list.hpp"

// Builds a child node's open goals from its parent's list: close one call site and add new goals under
// fresh ids.
struct pud_goal_resolver {
    pud_goal_list resolve_goal(const pud_goal_list& goals, pud_body_goal_id closed_goal, const std::vector<const expr*>& added_goals) const;
};

#endif
