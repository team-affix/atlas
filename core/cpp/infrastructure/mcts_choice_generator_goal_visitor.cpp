#include "../../hpp/infrastructure/mcts_choice_generator_goal_visitor.hpp"

mcts_choice_generator_goal_visitor::mcts_choice_generator_goal_visitor(size_t goal_count) {
    internal_choices.reserve(goal_count);
}

void mcts_choice_generator_goal_visitor::visit(const std::pair<const goal_lineage* const, std::unique_ptr<goal>>& entry) {
    internal_choices.push_back(entry.first);
}

const std::vector<mcts_choice>& mcts_choice_generator_goal_visitor::choices() const {
    return internal_choices;
}
