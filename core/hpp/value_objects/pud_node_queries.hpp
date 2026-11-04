#ifndef PUD_NODE_QUERIES_HPP
#define PUD_NODE_QUERIES_HPP

#include <immer/box.hpp>
#include <immer/map.hpp>
#include "value_objects/pud_body_goal_id.hpp"
#include "infrastructure/pud_mhq_head_owner.hpp"

// The MHQ head a live node holds for each of its open goals. A child copies its parent's map, so it shares the
// owners of every goal it neither forked nor closed.
template<typename IRemoveQuery>
using pud_node_queries = immer::map<pud_body_goal_id, immer::box<pud_mhq_head_owner<IRemoveQuery>>>;

#endif
