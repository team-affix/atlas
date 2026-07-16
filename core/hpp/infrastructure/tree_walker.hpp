#ifndef TREE_WALKER_HPP
#define TREE_WALKER_HPP

#include <cstddef>
#include <unordered_map>
#include "value_objects/mcts_choice.hpp"
#include "value_objects/mcts_choice_hash.hpp"
#include "value_objects/mcts_tree_node_id.hpp"

struct tree_walker {
    tree_walker();

    mcts_tree_node_id walk(mcts_tree_node_id node, const mcts_choice& choice);

private:
    using map_t = std::unordered_map<mcts_choice, mcts_tree_node_id, mcts_choice_hash>;

    std::unordered_map<mcts_tree_node_id, map_t> edges_;
    mcts_tree_node_id next_id_;
};

#endif
