#ifndef I_MCTS_CHOOSE_HPP
#define I_MCTS_CHOOSE_HPP

#include <vector>
#include "value_objects/mcts_choice.hpp"

struct i_mcts_choose {
    virtual ~i_mcts_choose() = default;
    virtual mcts_choice choose(const std::vector<mcts_choice>&) = 0;
};

#endif
