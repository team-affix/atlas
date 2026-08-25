#include "infrastructure/genius_fc_uct_manifest.hpp"

genius_fc_uct_manifest::genius_fc_uct_manifest(
    visits_table_t& get_visits,
    visits_table_t& set_visits,
    value_table_t& get_value,
    value_table_t& set_value,
    walker_t& walker,
    rollout_t& rollout,
    exploration_constant_t& exploration_constant,
    value_delta_t& delta,
    mcts_scope_node_id root)
    : walker(walker)
    , rollout(rollout)
    , exploration_constant(exploration_constant)
    , delta(delta)
    , value_update(get_value, set_value, this->delta)
    , cursor(root)
    , backprop_path(root)
    , visit_creditor(backprop_path, get_visits, set_visits)
    , value_creditor(visit_creditor, backprop_path, value_update)
    , policy(get_visits, get_value, this->walker, this->exploration_constant)
    , in_rollout()
    , chooser(get_visits, this->walker, policy, this->rollout,
              cursor, cursor, backprop_path,
              in_rollout, in_rollout)
    , terminator(backprop_path, value_creditor, backprop_path, backprop_path,
                 cursor, in_rollout, root)
{}
