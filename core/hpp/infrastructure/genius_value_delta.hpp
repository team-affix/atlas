#ifndef GENIUS_VALUE_DELTA_HPP
#define GENIUS_VALUE_DELTA_HPP

#include <set>
#include <utility>
#include "value_objects/lineage.hpp"

// genius_value_delta
//
// IGetValueDelta implementation for the genius solver.  The reward assigned to
// a node on the backprop path depends on what kind of decision was just made:
//
//   node.second == nullptr  (between-decisions: a rule choice was just committed)
//     → horizon reward (cumulative grounded weight)
//
//   node.second != nullptr  (goal-targeting: a goal choice was just committed)
//     → ridge reward (-decision_memory_size)
//
// Both rewards are queried lazily from their respective collaborators each time
// get_value_delta is called, so they reflect the solver state at termination.

template<typename IRidgeReward, typename IHorizonReward>
struct genius_value_delta {
    using node_id_t = std::pair<std::set<const resolution_lineage*>, const goal_lineage*>;

    genius_value_delta(IRidgeReward&, IHorizonReward&);

    double get_value_delta(const node_id_t&) const;

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
double genius_value_delta<IRR, IHR>::get_value_delta(const node_id_t& node) const {
    if (node.second == nullptr)
        return horizon_reward_.compute_mcts_reward();
    return ridge_reward_.compute_mcts_reward();
}

#endif
