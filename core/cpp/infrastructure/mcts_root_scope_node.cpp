#include "infrastructure/mcts_root_scope_node.hpp"

mcts_root_scope_node::mcts_root_scope_node() {}

mcts_scope_node_id mcts_root_scope_node::get_mcts_root_node() const {
    return mcts_scope_node_id{{}, nullptr};
}
