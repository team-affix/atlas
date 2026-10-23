#ifndef PUD_LEAF_STATE_HPP
#define PUD_LEAF_STATE_HPP

#include <immer/map.hpp>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_structure_watch.hpp"
#include "value_objects/pud_ubs_state.hpp"

// What a live leaf carries: the bindings of its rule, the watch over its open goals, and the query it
// holds for each open goal.
struct pud_leaf_state {
    pud_ubs_state bindings;
    pud_structure_watch watch;
    immer::map<pud_body_goal_id, pud_mhq_head_id> queries;
    bool operator==(const pud_leaf_state&) const = default;
};

#endif
