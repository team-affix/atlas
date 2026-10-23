#ifndef PUD_GOAL_QUERIES_HPP
#define PUD_GOAL_QUERIES_HPP

#include <immer/map.hpp>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"

// The query a leaf holds for each open goal, and whether some goal's query found no candidate, which
// refutes the leaf.
struct pud_goal_queries {
    immer::map<pud_body_goal_id, pud_mhq_head_id> heads;
    bool refuted;
    bool operator==(const pud_goal_queries&) const = default;
};

#endif
