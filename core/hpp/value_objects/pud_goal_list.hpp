#ifndef PUD_GOAL_LIST_HPP
#define PUD_GOAL_LIST_HPP

#include <immer/map.hpp>
#include "value_objects/expr.hpp"
#include "value_objects/pud_body_goal_id.hpp"

// The open body goals of a node by id, and the id its next added goal gets. A child closes its
// parent's call site and adds the goals it brings, so a goal keeps its id at every node below the one
// that added it, and no id is ever reused.
struct pud_goal_list {
    immer::map<pud_body_goal_id, const expr*> goals;
    pud_body_goal_id next_id;

    pud_goal_list closing(pud_body_goal_id goal) const;
    pud_goal_list adding(const expr* goal) const;
    bool operator==(const pud_goal_list&) const = default;
};

#endif
