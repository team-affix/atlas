#ifndef PUD_GOAL_TRAILS_HPP
#define PUD_GOAL_TRAILS_HPP

#include <compare>
#include <vector>
#include "value_objects/pud_candidate_trail.hpp"

// The trails a query of one goal left, in visible-axiom order. An axiom without a trail had no candidate and never
// will. Forked trails were made before the goal's structure changed, so each must be replayed again.
struct pud_goal_trails {
    std::vector<pud_candidate_trail> candidates;
    bool forked;
    auto operator<=>(const pud_goal_trails&) const = default;
};

#endif
