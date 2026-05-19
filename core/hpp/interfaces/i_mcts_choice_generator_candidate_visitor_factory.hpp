#ifndef I_MCTS_CHOICE_GENERATOR_CANDIDATE_VISITOR_FACTORY_HPP
#define I_MCTS_CHOICE_GENERATOR_CANDIDATE_VISITOR_FACTORY_HPP

#include "../interfaces/i_factory.hpp"
#include "../interfaces/i_mcts_choice_generator_candidate_visitor.hpp"
#include "../value_objects/mcts_choice.hpp"

struct i_mcts_choice_generator_candidate_visitor_factory : i_factory<i_mcts_choice_generator_candidate_visitor, std::vector<mcts_choice>&> {
    virtual ~i_mcts_choice_generator_candidate_visitor_factory() = default;
};

#endif
