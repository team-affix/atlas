#include "infrastructure/dbuct_manifest.hpp"

dbuct_manifest::dbuct_manifest(
    visits_table_t& get_visits,
    visits_table_t& set_visits,
    value_table_t& get_value,
    value_table_t& set_value,
    std::mt19937& rnd_gen,
    double exploration_constant,
    double grant_k,
    mcts_tree_node_id root)
    : walker()
    , rollout(rnd_gen)
    , exploration_constant(exploration_constant)
    , delta()
    , grant(get_visits, grant_k)
    , frame_stack(monte_carlo::dbuct_frame<mcts_tree_node_id>(root, std::numeric_limits<size_t>::max()))
    , visit_adder(frame_stack, get_visits, set_visits)
    , frame_stack_controller(frame_stack, frame_stack, frame_stack, visit_adder)
    , value_stack(monte_carlo::dbuct_value_frame<mcts_tree_node_id, double>(root))
    , value_adder(value_stack, get_value, set_value)
    , value_stack_controller(frame_stack_controller, frame_stack_controller,
                             value_stack, value_stack, value_stack, value_adder)
    , visit_creditor(visit_adder)
    , value_creditor(visit_creditor, value_stack, value_adder, delta)
    , policy(get_visits, get_value, walker, this->exploration_constant)
    , in_rollout()
    , chooser(get_visits, grant,
              value_stack_controller, frame_stack,
              walker, policy, rollout,
              in_rollout, in_rollout)
    , terminator(value_stack_controller, frame_stack, value_creditor, in_rollout)
{}
