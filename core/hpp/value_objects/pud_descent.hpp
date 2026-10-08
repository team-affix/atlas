#ifndef PUD_DESCENT_HPP
#define PUD_DESCENT_HPP

#include <cstddef>
#include <cstdint>
#include <immer/map.hpp>
#include <immer/set.hpp>
#include "pud_node.hpp"
#include "value_objects/body_goal_id.hpp"
#include "value_objects/framed_expr.hpp"

struct pud_descent {
    uint32_t frame_offset;
    uint32_t lvc;
    size_t bgc;
    const pud_node* node;
    immer::set<uint32_t> touched_caller_reps;
    immer::map<uint32_t, framed_expr> bindings;
    immer::map<body_goal_id, const expr*> pending_body_goals;
    auto operator<=>(const pud_descent&) const = default;
};

#endif
