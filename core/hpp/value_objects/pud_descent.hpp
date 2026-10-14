#ifndef PUD_DESCENT_HPP
#define PUD_DESCENT_HPP

#include <cstddef>
#include <cstdint>
#include <immer/map.hpp>
#include <immer/set.hpp>
#include "value_objects/pud_node_id.hpp"
#include "value_objects/body_goal_id.hpp"
#include "value_objects/pud_ubs_state.hpp"

struct pud_descent {
    uint32_t frame_offset;
    uint32_t lvc;
    size_t bgc;
    pud_node_id node;
    immer::set<uint32_t> touched_caller_reps;
    pud_ubs_state ubs_state;
    immer::map<body_goal_id, const expr*> pending_body_goals;
    auto operator<=>(const pud_descent&) const = default;
};

#endif
