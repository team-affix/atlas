#include "infrastructure/check_mcts_choice_is_rule_choice.hpp"

#include <variant>

check_mcts_choice_is_rule_choice::check_mcts_choice_is_rule_choice() {}

bool check_mcts_choice_is_rule_choice::check_is_rule_choice(const mcts_choice& choice) const {
    return std::holds_alternative<rule_id>(choice);
}
