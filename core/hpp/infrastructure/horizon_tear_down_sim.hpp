#ifndef HORIZON_TEAR_DOWN_SIM_HPP
#define HORIZON_TEAR_DOWN_SIM_HPP

#include "infrastructure/tear_down_sim.hpp"
#include "interfaces/i_clear_goal_weights.hpp"
#include "interfaces/i_clear_grounded_weight.hpp"

struct horizon_tear_down_sim : tear_down_sim {
    horizon_tear_down_sim(locator& loc);
    void tear_down() override;
private:
    i_clear_goal_weights& clear_goal_weights_;
    i_clear_grounded_weight& clear_grounded_weight_;
};

#endif
