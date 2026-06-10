#include "infrastructure/ridge_reward.hpp"

ridge_reward::ridge_reward(locator& loc)
    : decision_count_(loc.locate<i_get_decision_count>()) {}

double ridge_reward::compute_mcts_reward() const {
    return -static_cast<double>(decision_count_.count());
}
