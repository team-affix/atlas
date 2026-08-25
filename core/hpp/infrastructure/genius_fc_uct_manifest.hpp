#ifndef GENIUS_FC_UCT_MANIFEST_HPP
#define GENIUS_FC_UCT_MANIFEST_HPP

#include <map>
#include <random>
#include <vector>
#include "infrastructure/cumulative_grounded_weight.hpp"
#include "infrastructure/decision_memory.hpp"
#include "infrastructure/genius_exploration_constant.hpp"
#include "infrastructure/genius_value_delta.hpp"
#include "infrastructure/horizon_reward.hpp"
#include "infrastructure/in_rollout_flag.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/ridge_reward.hpp"
#include "infrastructure/rp_fewer_candidate_goal_rollout.hpp"
#include "infrastructure/rp_heuristic_rollout.hpp"
#include "infrastructure/rp_srt_active_goals.hpp"
#include "infrastructure/rp_uniform_rule_rollout.hpp"
#include "infrastructure/scope_walker.hpp"
#include "infrastructure/srt_active_goals.hpp"
#include "infrastructure/ucb1.hpp"
#include "infrastructure/uct_backprop_path.hpp"
#include "infrastructure/uct_chooser.hpp"
#include "infrastructure/uct_cursor.hpp"
#include "infrastructure/uct_terminator.hpp"
#include "infrastructure/uct_value_creditor.hpp"
#include "infrastructure/uct_visit_creditor.hpp"
#include "infrastructure/uniform_value_update.hpp"
#include "infrastructure/value_table.hpp"
#include "infrastructure/visits_table.hpp"
#include "value_objects/mcts_choice.hpp"
#include "value_objects/mcts_scope_node_id.hpp"

struct genius_fc_uct_manifest {
    using choices_t              = std::vector<mcts_choice>;
    using visits_table_t         = monte_carlo::visits_table<mcts_scope_node_id, std::map>;
    using value_table_t          = monte_carlo::value_table<mcts_scope_node_id, double, std::map>;
    using walker_t               = scope_walker<lineage_pool>;
    using rp_srt_active_goals_t  = rp_srt_active_goals<
                                        srt_active_goals, srt_active_goals,
                                        srt_active_goals, srt_active_goals,
                                        srt_active_goals>;
    using goal_rollout_t         = rp_fewer_candidate_goal_rollout<rp_srt_active_goals_t>;
    using rule_rollout_t         = rp_uniform_rule_rollout<std::mt19937>;
    using rollout_t              = rp_heuristic_rollout<goal_rollout_t, rule_rollout_t>;
    using exploration_constant_t = genius_exploration_constant<srt_active_goals>;
    using value_delta_t          = genius_value_delta<ridge_reward<decision_memory>, horizon_reward<cumulative_grounded_weight>>;
    using value_update_t         = monte_carlo::uniform_value_update<mcts_scope_node_id, value_table_t, value_table_t, value_delta_t>;
    using cursor_t               = monte_carlo::uct_cursor<mcts_scope_node_id>;
    using path_t                 = monte_carlo::uct_backprop_path<mcts_scope_node_id>;
    using visit_creditor_t       = monte_carlo::uct_visit_creditor<mcts_scope_node_id, path_t, visits_table_t, visits_table_t>;
    using value_creditor_t       = monte_carlo::uct_value_creditor<visit_creditor_t, path_t, value_update_t>;
    using policy_t               = monte_carlo::ucb1<mcts_scope_node_id, mcts_choice, double,
                                        visits_table_t, value_table_t, walker_t,
                                        exploration_constant_t, choices_t, choices_t>;
    using chooser_t              = monte_carlo::uct_chooser<mcts_scope_node_id, mcts_choice, visits_table_t, walker_t,
                                               choices_t, choices_t,
                                               policy_t, rollout_t,
                                               cursor_t, cursor_t, path_t,
                                               monte_carlo::in_rollout_flag, monte_carlo::in_rollout_flag>;
    using terminator_t           = monte_carlo::uct_terminator<mcts_scope_node_id,
                                                  path_t,
                                                  value_creditor_t,
                                                  path_t, path_t,
                                                  cursor_t,
                                                  monte_carlo::in_rollout_flag>;

    genius_fc_uct_manifest(visits_table_t&, visits_table_t&, value_table_t&, value_table_t&,
                           walker_t&, rollout_t&, exploration_constant_t&, value_delta_t&,
                           mcts_scope_node_id);

    walker_t&                        walker;
    rollout_t&                       rollout;
    exploration_constant_t&          exploration_constant;
    value_delta_t&                   delta;
    value_update_t                   value_update;
    cursor_t                         cursor;
    path_t                           backprop_path;
    visit_creditor_t                 visit_creditor;
    value_creditor_t                 value_creditor;
    policy_t                         policy;
    monte_carlo::in_rollout_flag     in_rollout;
    chooser_t                        chooser;
    terminator_t                     terminator;
};

#endif
