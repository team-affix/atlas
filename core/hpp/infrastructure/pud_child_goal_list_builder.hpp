#ifndef PUD_CHILD_GOAL_LIST_BUILDER_HPP
#define PUD_CHILD_GOAL_LIST_BUILDER_HPP

#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_goal_list.hpp"

struct pud_child_goal_list_builder {
    pud_goal_list build(const pud_goal_list& goals, pud_body_goal_id closed_goal, const std::vector<const expr*>& added_goals) const;
};

#endif
