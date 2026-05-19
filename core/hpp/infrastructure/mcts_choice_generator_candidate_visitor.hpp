#ifndef MCTS_CHOICE_GENERATOR_CANDIDATE_VISITOR_HPP
#define MCTS_CHOICE_GENERATOR_CANDIDATE_VISITOR_HPP

#include <vector>
#include "../interfaces/i_mcts_choice_generator_candidate_visitor.hpp"
#include "../value_objects/mcts_choice.hpp"

struct mcts_choice_generator_candidate_visitor : i_mcts_choice_generator_candidate_visitor {
    mcts_choice_generator_candidate_visitor(size_t);
    void visit(const rule*) override;
    const std::vector<mcts_choice>& choices() const;
private:
    std::vector<mcts_choice> internal_choices;
};

#endif
