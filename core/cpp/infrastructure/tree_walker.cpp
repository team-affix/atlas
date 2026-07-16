#include "infrastructure/tree_walker.hpp"

tree_walker::tree_walker() : next_id_(0) {}

mcts_tree_node_id tree_walker::walk(mcts_tree_node_id node, const mcts_choice& choice) {
    map_t& children = edges_[node];
    auto it = children.find(choice);
    if (it != children.end())
        return it->second;
    mcts_tree_node_id child = ++next_id_;
    children.emplace(choice, child);
    return child;
}
