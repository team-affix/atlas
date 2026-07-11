#include "infrastructure/dbuct_tree_walker.hpp"

size_t dbuct_tree_walker::make_root() {
    return 0;
}

size_t dbuct_tree_walker::walk(size_t node, const mcts_choice& choice) {
    children_t& children = edges_[node];
    auto it = children.find(choice);
    if (it != children.end())
        return it->second;
    node_id_t child = ++next_id_;
    children.emplace(choice, child);
    return child;
}
