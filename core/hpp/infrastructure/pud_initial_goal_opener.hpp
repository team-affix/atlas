#ifndef PUD_INITIAL_GOAL_OPENER_HPP
#define PUD_INITIAL_GOAL_OPENER_HPP

#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/pud_goal_list.hpp"

struct pud_initial_goal_opener {
    pud_goal_list open_goals(const std::vector<const expr*>& opened_goals) const;
};

#endif
