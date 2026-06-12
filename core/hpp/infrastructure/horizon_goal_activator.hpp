#ifndef HORIZON_GOAL_ACTIVATOR_HPP
#define HORIZON_GOAL_ACTIVATOR_HPP

#include "infrastructure/locator.hpp"
#include "infrastructure/goal_activator.hpp"
#include "interfaces/i_goal_activator.hpp"
#include "interfaces/i_set_goal_weight.hpp"
#include "interfaces/i_get_goal_weight.hpp"
#include "interfaces/i_get_rule.hpp"

struct horizon_goal_activator : i_goal_activator {
    horizon_goal_activator(locator& loc);
    void activate(const goal_lineage*) override;
private:
    goal_activator& goal_activator_;
    i_set_goal_weight& set_goal_weight_;
    i_get_goal_weight& get_goal_weight_;
    i_get_rule& get_rule_;
};

#endif
