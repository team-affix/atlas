#ifndef MCTS_CHOICE_GENERATOR_GOAL_VISITOR_HPP
#define MCTS_CHOICE_GENERATOR_GOAL_VISITOR_HPP

#include <memory>
#include <utility>
#include <vector>
#include "../domain/interfaces/i_visitor.hpp"
#include "../domain/value_objects/goal.hpp"
#include "../domain/value_objects/lineage.hpp"
#include "mcts_choice.hpp"

struct mcts_choice_generator_goal_visitor : i_visitor<const std::pair<const goal_lineage* const, std::unique_ptr<goal>>&> {
    mcts_choice_generator_goal_visitor(size_t);
    void visit(const std::pair<const goal_lineage* const, std::unique_ptr<goal>>&) override;
    const std::vector<mcts_choice>& choices() const;
private:
    std::vector<mcts_choice> internal_choices;
};

#endif
