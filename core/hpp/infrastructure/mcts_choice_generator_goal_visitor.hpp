#ifndef MCTS_CHOICE_GENERATOR_GOAL_VISITOR_HPP
#define MCTS_CHOICE_GENERATOR_GOAL_VISITOR_HPP

#include <vector>
#include "../domain/interfaces/i_visitor.hpp"
#include "../domain/value_objects/lineage.hpp"
#include "mcts_choice.hpp"

struct mcts_choice_generator_goal_visitor : i_visitor<const goal_lineage*> {
    mcts_choice_generator_goal_visitor(size_t);
    void visit(const goal_lineage*) override;
    const std::vector<mcts_choice>& choices() const;
private:
    std::vector<mcts_choice> internal_choices;
};

#endif
