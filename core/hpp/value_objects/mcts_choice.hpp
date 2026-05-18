#ifndef MCTS_CHOICE_HPP
#define MCTS_CHOICE_HPP

#include <variant>
#include "lineage.hpp"

using mcts_choice = std::variant<const goal_lineage*, size_t>;

#endif
