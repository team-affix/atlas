#ifndef PUD_QUERY_DESCENT_HPP
#define PUD_QUERY_DESCENT_HPP

#include "value_objects/pud_node_id.hpp"

// A query's replay positioned at a node of the tree being queried.
template<typename ReplayDescent>
struct pud_query_descent {
    pud_node_id node;
    ReplayDescent replay;
};

#endif
