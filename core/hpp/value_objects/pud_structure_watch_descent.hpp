#ifndef PUD_STRUCTURE_WATCH_DESCENT_HPP
#define PUD_STRUCTURE_WATCH_DESCENT_HPP

#include <cstdint>
#include <immer/map.hpp>
#include <immer/set.hpp>
#include "value_objects/pud_mhsw_head_id.hpp"

struct pud_structure_watch_descent {
    pud_mhsw_head_id next_head_id;
    immer::map<uint32_t, immer::set<pud_mhsw_head_id>> rep_to_heads;
    immer::map<pud_mhsw_head_id, immer::set<uint32_t>> head_to_reps;
    auto operator<=>(const pud_structure_watch_descent&) const = default;
};

#endif
