#ifndef DBUCT_TREE_WALKER_HPP
#define DBUCT_TREE_WALKER_HPP

#include <cstddef>
#include <unordered_map>
#include "value_objects/mcts_choice.hpp"
#include "value_objects/mcts_choice_hash.hpp"

struct dbuct_tree_walker {
    size_t walk(size_t node, const mcts_choice& choice);

private:
    using children_t = std::unordered_map<mcts_choice, size_t, mcts_choice_hash>;
    std::unordered_map<size_t, children_t> edges_;
    size_t next_id_ = 0;
};

#endif
