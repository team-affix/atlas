#include "infrastructure/horizon_tear_down_sim.hpp"

horizon_tear_down_sim::horizon_tear_down_sim(locator& loc)
    : tear_down_sim(loc),
      clear_goal_weights_(loc.locate<i_clear_goal_weights>()),
      clear_grounded_weight_(loc.locate<i_clear_grounded_weight>()) {}

void horizon_tear_down_sim::tear_down() {
    clear_goal_weights_.clear_goal_weights();
    clear_grounded_weight_.clear();
    tear_down_sim::tear_down();
}
