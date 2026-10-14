#ifndef PUD_BODY_GOAL_PQ_DESCENT_HPP
#define PUD_BODY_GOAL_PQ_DESCENT_HPP

#include <immer/box.hpp>
#include <immer/map.hpp>
#include "infrastructure/persistent_query.hpp"
#include "value_objects/body_goal_id.hpp"

struct pud_body_goal_pq_descent {
    immer::map<body_goal_id, immer::box<persistent_query>> body_goal_pqs;
    auto operator<=>(const pud_body_goal_pq_descent&) const = default;
};

#endif
