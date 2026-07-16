#ifndef GENIUS_EXPLORATION_CONSTANT_HPP
#define GENIUS_EXPLORATION_CONSTANT_HPP

#include "value_objects/mcts_scope_node_id.hpp"

template<typename IIsActiveGoal>
struct genius_exploration_constant {
    genius_exploration_constant(IIsActiveGoal& is_active_goal,
                                double ridge_exploration_constant,
                                double horizon_exploration_constant);

    double get_exploration_constant(const mcts_scope_node_id& node) const;

private:
    IIsActiveGoal& is_active_goal_;
    double ridge_exploration_constant_;
    double horizon_exploration_constant_;
};

template<typename IIsActiveGoal>
genius_exploration_constant<IIsActiveGoal>::genius_exploration_constant(
    IIsActiveGoal& is_active_goal,
    double ridge_exploration_constant,
    double horizon_exploration_constant)
    : is_active_goal_(is_active_goal)
    , ridge_exploration_constant_(ridge_exploration_constant)
    , horizon_exploration_constant_(horizon_exploration_constant)
{}

template<typename IIsActiveGoal>
double genius_exploration_constant<IIsActiveGoal>::get_exploration_constant(
    const mcts_scope_node_id& node) const {
    if (is_active_goal_.is_active_goal(node.second))
        return horizon_exploration_constant_;
    return ridge_exploration_constant_;
}

#endif
