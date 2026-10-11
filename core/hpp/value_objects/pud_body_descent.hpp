#ifndef PUD_BODY_DESCENT_HPP
#define PUD_BODY_DESCENT_HPP

#include "immer/map.hpp"
#include "value_objects/body_goal_id.hpp"
#include "value_objects/expr.hpp"

struct pud_body_descent {
    immer::map<body_goal_id, const expr*> body_goals;
    body_goal_id next_body_goal_id;
};

#endif
