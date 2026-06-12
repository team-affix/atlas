#include "infrastructure/horizon_reward.hpp"

horizon_reward::horizon_reward(locator& loc)
    : grounded_weight_(loc.locate<i_get_grounded_weight>()) {}

double horizon_reward::compute_mcts_reward() const {
    return grounded_weight_.get();
}
