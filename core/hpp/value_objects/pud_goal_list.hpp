#ifndef PUD_GOAL_LIST_HPP
#define PUD_GOAL_LIST_HPP

#include <immer/map.hpp>
#include "value_objects/expr.hpp"
#include "value_objects/pud_body_goal_id.hpp"

struct pud_goal_list {
    immer::map<pud_body_goal_id, const expr*> goals;
    pud_body_goal_id next_id;

    bool operator==(const pud_goal_list&) const = default;
};

#endif
