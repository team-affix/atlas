#ifndef PUD_QUERY_NODE_HPP
#define PUD_QUERY_NODE_HPP

#include <memory>
#include <vector>
#include <cstdint>
#include "pud_node.hpp"

struct pud_query_node {
    const pud_node* node;
    std::vector<uint32_t> touched_caller_reps;
    std::shared_ptr<pud_query_node> parent;
};

#endif
