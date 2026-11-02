#ifndef PUD_ROOT_GOAL_LIST_BUILDER_HPP
#define PUD_ROOT_GOAL_LIST_BUILDER_HPP

#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/pud_goal_list.hpp"

struct pud_root_goal_list_builder {
    pud_goal_list build(const std::vector<const expr*>& opened_goals) const;
};

#endif
