#include "infrastructure/mcts_root_tree_node.hpp"

mcts_root_tree_node::mcts_root_tree_node() {}

mcts_tree_node_id mcts_root_tree_node::get_mcts_root_node() const {
    return 0;
}
