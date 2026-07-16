#ifndef MCTS_ROOT_TREE_NODE_HPP
#define MCTS_ROOT_TREE_NODE_HPP

#include "value_objects/mcts_tree_node_id.hpp"

struct mcts_root_tree_node {
    mcts_root_tree_node();
    mcts_tree_node_id get_mcts_root_node() const;
};

#endif
