#include "infrastructure/phoenix_fc_uct_manifest.hpp"

phoenix_fc_uct_manifest::phoenix_fc_uct_manifest(
    visits_table_t& get_visits,
    visits_table_t& set_visits,
    value_table_t& get_value,
    value_table_t& set_value,
    tree_walker& walker,
    rollout_t& rollout,
    exploration_constant_t& exploration_constant,
    value_delta_t& delta,
    std::mt19937& rnd_gen,
    mcts_tree_node_id root)
    : walker(walker)
    , rollout(rollout)
    , index_sample(rnd_gen)
    , rule_policy(this->index_sample)
    , exploration_constant(exploration_constant)
    , delta(delta)
    , value_update(get_value, set_value, this->delta)
    , cursor(root)
    , backprop_path(root)
    , visit_creditor(backprop_path, get_visits, set_visits)
    , value_creditor(visit_creditor, backprop_path, value_update)
    , goal_policy(get_visits, get_value, this->walker, this->exploration_constant)
    , check_is_rule_choice()
    , policy(this->check_is_rule_choice, this->goal_policy, this->rule_policy)
    , in_rollout()
    , chooser(get_visits, this->walker, policy, this->rollout,
              cursor, cursor, backprop_path,
              in_rollout, in_rollout)
    , terminator(backprop_path, value_creditor, backprop_path, backprop_path,
                 cursor, in_rollout, root)
{}
