#ifndef CHECK_MCTS_CHOICE_IS_RULE_CHOICE_HPP
#define CHECK_MCTS_CHOICE_IS_RULE_CHOICE_HPP

#include "value_objects/mcts_choice.hpp"

struct check_mcts_choice_is_rule_choice {
    check_mcts_choice_is_rule_choice();
    bool check_is_rule_choice(const mcts_choice& choice) const;
};

#endif
