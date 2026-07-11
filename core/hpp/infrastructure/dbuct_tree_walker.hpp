#ifndef DBUCT_TREE_WALKER_HPP
#define DBUCT_TREE_WALKER_HPP

#include <cstddef>
#include <unordered_map>
#include "value_objects/mcts_choice.hpp"
#include "value_objects/mcts_choice_hash.hpp"

struct dbuct_tree_walker {
    using node_id_t = size_t;

    static constexpr bool use_unordered_tables = true;

    node_id_t walk(node_id_t node, const mcts_choice& choice);

    static node_id_t make_root();

private:
    using children_t = std::unordered_map<mcts_choice, node_id_t, mcts_choice_hash>;

    std::unordered_map<node_id_t, children_t> edges_;
    node_id_t next_id_ = 0;
};

#endif
