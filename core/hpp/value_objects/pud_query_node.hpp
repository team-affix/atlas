#ifndef PUD_QUERY_NODE_HPP
#define PUD_QUERY_NODE_HPP

#include <memory>
#include <vector>
#include <cstdint>
#include <immer/map.hpp>
#include "pud_node.hpp"
#include "value_objects/framed_expr.hpp"

struct pud_query_node {
    uint32_t frame_offset;
    const pud_node* node;
    std::vector<uint32_t> touched_caller_reps;
    std::shared_ptr<pud_query_node> parent;
    immer::map<uint32_t, framed_expr> bindings;
    uint32_t lvc;
};

#endif
