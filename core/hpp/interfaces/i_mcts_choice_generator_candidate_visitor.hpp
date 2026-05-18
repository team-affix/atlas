#ifndef I_MCTS_CHOICE_GENERATOR_CANDIDATE_VISITOR_HPP
#define I_MCTS_CHOICE_GENERATOR_CANDIDATE_VISITOR_HPP

#include "../interfaces/i_visitor.hpp"
#include "../value_objects/lineage.hpp"

struct i_mcts_choice_generator_candidate_visitor : i_visitor<const resolution_lineage*> {
    virtual ~i_mcts_choice_generator_candidate_visitor() = default;
};

#endif
