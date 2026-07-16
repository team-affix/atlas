#ifndef MCTS_ROOT_SCOPE_NODE_HPP
#define MCTS_ROOT_SCOPE_NODE_HPP

#include "value_objects/mcts_scope_node_id.hpp"

struct mcts_root_scope_node {
    mcts_root_scope_node();
    mcts_scope_node_id get_mcts_root_node() const;
};

#endif
