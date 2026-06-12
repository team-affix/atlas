#ifndef GOAL_WEIGHTS_HPP
#define GOAL_WEIGHTS_HPP

#include <unordered_map>
#include "interfaces/i_get_goal_weight.hpp"
#include "interfaces/i_set_goal_weight.hpp"
#include "interfaces/i_erase_goal_weight.hpp"
#include "interfaces/i_clear_goal_weights.hpp"

struct goal_weights
    : i_get_goal_weight
    , i_set_goal_weight
    , i_erase_goal_weight
    , i_clear_goal_weights {
    double get(const goal_lineage*) const override;
    void set(const goal_lineage*, double) override;
    void erase(const goal_lineage*) override;
    void clear_goal_weights() override;
private:
    std::unordered_map<const goal_lineage*, double> weights_;
};

#endif
