#ifndef PUD_QUERY_NODE_HPP
#define PUD_QUERY_NODE_HPP

#include <cstddef>
#include <cstdint>
#include <immer/map.hpp>
#include <immer/set.hpp>
#include "pud_node.hpp"
#include "value_objects/body_goal_id.hpp"
#include "value_objects/framed_expr.hpp"

struct pud_query_node {
    uint32_t frame_offset;
    const pud_node* node;
    immer::set<uint32_t> touched_caller_reps;
    immer::map<uint32_t, framed_expr> bindings;
    uint32_t lvc;
    immer::map<body_goal_id, const expr*> pending_body_goals;
    size_t bgc;
};

#endif
