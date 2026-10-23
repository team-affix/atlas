#ifndef PUD_GOAL_LIST_HPP
#define PUD_GOAL_LIST_HPP

#include <vector>
#include <immer/flex_vector.hpp>
#include "value_objects/expr.hpp"
#include "value_objects/pud_body_goal.hpp"
#include "value_objects/pud_body_goal_id.hpp"

// The open body goals of a node in id order, and the id its next added goal gets. A child closes its
// parent's call site and appends the goals it adds, so ids stay ascending along every path.
struct pud_goal_list {
    immer::flex_vector<pud_body_goal> goals;
    pud_body_goal_id next_id;

    pud_goal_list closing(pud_body_goal_id goal) const;
    pud_goal_list adding(const std::vector<const expr*>& added) const;
    bool operator==(const pud_goal_list&) const = default;
};

#endif
