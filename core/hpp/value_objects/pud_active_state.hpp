#ifndef PUD_ACTIVE_STATE_HPP
#define PUD_ACTIVE_STATE_HPP

#include <cstdint>
#include <immer/map.hpp>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_structure_watch.hpp"
#include "value_objects/pud_ubs_state.hpp"

struct pud_active_state {
    uint32_t lvc;
    pud_ubs_state bindings;
    pud_structure_watch watch;
    immer::map<pud_body_goal_id, pud_mhq_head_id> queries;
    bool operator==(const pud_active_state&) const = default;
};

#endif
