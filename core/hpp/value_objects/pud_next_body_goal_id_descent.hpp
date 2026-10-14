#ifndef PUD_NEXT_BODY_GOAL_ID_DESCENT_HPP
#define PUD_NEXT_BODY_GOAL_ID_DESCENT_HPP

#include "value_objects/body_goal_id.hpp"

struct pud_next_body_goal_id_descent {
    body_goal_id next_body_goal_id;
    auto operator<=>(const pud_next_body_goal_id_descent&) const = default;
};

#endif
