#ifndef GENIUS_VALUE_DELTA_HPP
#define GENIUS_VALUE_DELTA_HPP

#include "value_objects/mcts_node_id.hpp"

template<typename IRidgeReward, typename IHorizonReward>
struct genius_value_delta {
    genius_value_delta(IRidgeReward&, IHorizonReward&);

    void   set_value(double) {}
    double get_value_delta(const mcts_node_id&) const;

private:
    IRidgeReward&   ridge_reward_;
    IHorizonReward& horizon_reward_;
};

template<typename IRR, typename IHR>
genius_value_delta<IRR, IHR>::genius_value_delta(IRR& ridge, IHR& horizon)
    : ridge_reward_(ridge)
    , horizon_reward_(horizon)
{}

template<typename IRR, typename IHR>
double genius_value_delta<IRR, IHR>::get_value_delta(const mcts_node_id& node) const {
    if (node.second == nullptr)
        return horizon_reward_.compute_mcts_reward();
    return ridge_reward_.compute_mcts_reward();
}

#endif
