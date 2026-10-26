#ifndef PUD_STRUCTURE_WATCH_HPP
#define PUD_STRUCTURE_WATCH_HPP

#include <cstdint>
#include <immer/map.hpp>
#include <immer/set.hpp>
#include "value_objects/pud_body_goal_id.hpp"

struct pud_structure_watch {
    immer::map<uint32_t, immer::set<pud_body_goal_id>> rep_to_goals;
    immer::map<pud_body_goal_id, immer::set<uint32_t>> goal_to_reps;
    bool operator==(const pud_structure_watch&) const = default;
};

#endif
