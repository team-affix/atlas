#include "infrastructure/dbuct_tree_walker.hpp"

dbuct_tree_walker::dbuct_tree_walker() : next_id_(0) {}


size_t dbuct_tree_walker::walk(size_t node, const mcts_choice& choice) {
    map_t& children = edges_[node];
    auto it = children.find(choice);
    if (it != children.end())
        return it->second;
    size_t child = ++next_id_;
    children.emplace(choice, child);
    return child;
}