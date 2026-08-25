#include "infrastructure/genius_dbuct_manifest.hpp"

genius_dbuct_manifest::genius_dbuct_manifest(
    visits_table_t& get_visits,
    visits_table_t& set_visits,
    value_table_t& get_value,
    value_table_t& set_value,
    walker_t& walker,
    rollout_t& rollout,
    exploration_constant_t& exploration_constant,
    value_delta_t& delta,
    double grant_k,
    mcts_scope_node_id root)
    : walker(walker)
    , rollout(rollout)
    , exploration_constant(exploration_constant)
    , delta(delta)
    , grant(get_visits, grant_k)
    , frame_stack(monte_carlo::dbuct_frame<mcts_scope_node_id>(root, std::numeric_limits<size_t>::max()))
    , visit_adder(frame_stack, get_visits, set_visits)
    , frame_stack_controller(frame_stack, frame_stack, frame_stack, visit_adder)
    , value_stack(monte_carlo::dbuct_value_frame<mcts_scope_node_id, double>(root))
    , value_adder(value_stack, get_value, set_value)
    , value_stack_controller(frame_stack_controller, frame_stack_controller,
                             value_stack, value_stack, value_stack, value_adder)
    , visit_creditor(visit_adder)
    , value_creditor(visit_creditor, value_stack, value_adder, this->delta)
    , policy(get_visits, get_value, this->walker, this->exploration_constant)
    , in_rollout()
    , chooser(get_visits, grant,
              value_stack_controller, frame_stack,
              this->walker, policy, this->rollout,
              in_rollout, in_rollout)
    , terminator(value_stack_controller, frame_stack, value_creditor, in_rollout)
{}
