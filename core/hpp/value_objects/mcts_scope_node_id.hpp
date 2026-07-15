#ifndef MCTS_SCOPE_NODE_ID_HPP
#define MCTS_SCOPE_NODE_ID_HPP

#include <set>
#include <utility>
#include "lineage.hpp"

using mcts_scope_node_id = std::pair<std::set<const resolution_lineage*>, const goal_lineage*>;

#endif
