#ifndef PUD_NODE_QUERIES_HPP
#define PUD_NODE_QUERIES_HPP

#include <immer/map.hpp>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"

// The MHQ head a live node holds for each of its open goals.
using pud_node_queries = immer::map<pud_body_goal_id, pud_mhq_head_id>;

#endif
