#ifndef I_MCTS_CHOICE_GENERATOR_GOAL_VISITOR_HPP
#define I_MCTS_CHOICE_GENERATOR_GOAL_VISITOR_HPP

#include "../interfaces/i_visitor.hpp"
#include "../value_objects/lineage.hpp"

struct i_mcts_choice_generator_goal_visitor : i_visitor<const goal_lineage*> {
    virtual ~i_mcts_choice_generator_goal_visitor() = default;
};

#endif
