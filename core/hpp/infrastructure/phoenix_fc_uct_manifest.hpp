#ifndef PHOENIX_FC_UCT_MANIFEST_HPP
#define PHOENIX_FC_UCT_MANIFEST_HPP

#include <random>
#include <unordered_map>
#include <vector>
#include "infrastructure/check_mcts_choice_is_rule_choice.hpp"
#include "infrastructure/in_rollout_flag.hpp"
#include "infrastructure/phoenix_policy_choose.hpp"
#include "infrastructure/random_policy_choose.hpp"
#include "infrastructure/rp_fewer_candidate_goal_rollout.hpp"
#include "infrastructure/rp_heuristic_rollout.hpp"
#include "infrastructure/rp_srt_active_goals.hpp"
#include "infrastructure/rp_uniform_rule_rollout.hpp"
#include "infrastructure/srt_active_goals.hpp"
#include "infrastructure/tree_walker.hpp"
#include "infrastructure/ucb1.hpp"
#include "infrastructure/uct_backprop_path.hpp"
#include "infrastructure/uct_chooser.hpp"
#include "infrastructure/uct_cursor.hpp"
#include "infrastructure/uct_terminator.hpp"
#include "infrastructure/uct_value_creditor.hpp"
#include "infrastructure/uct_visit_creditor.hpp"
#include "infrastructure/uniform_exploration_constant.hpp"
#include "infrastructure/uniform_index_sample.hpp"
#include "infrastructure/uniform_value_delta.hpp"
#include "infrastructure/uniform_value_update.hpp"
#include "infrastructure/value_table.hpp"
#include "infrastructure/visits_table.hpp"
#include "value_objects/mcts_choice.hpp"
#include "value_objects/mcts_tree_node_id.hpp"

struct phoenix_fc_uct_manifest {
    using choices_t               = std::vector<mcts_choice>;
    using visits_table_t          = monte_carlo::visits_table<mcts_tree_node_id, std::unordered_map>;
    using value_table_t           = monte_carlo::value_table<mcts_tree_node_id, double, std::unordered_map>;
    using rp_srt_active_goals_t   = rp_srt_active_goals<
                                        srt_active_goals, srt_active_goals,
                                        srt_active_goals, srt_active_goals,
                                        srt_active_goals>;
    using goal_rollout_t          = rp_fewer_candidate_goal_rollout<rp_srt_active_goals_t>;
    using rule_rollout_t          = rp_uniform_rule_rollout<std::mt19937>;
    using rollout_t               = rp_heuristic_rollout<goal_rollout_t, rule_rollout_t>;
    using index_sample_t          = uniform_index_sample<std::mt19937>;
    using rule_policy_t           = random_policy_choose<mcts_choice, index_sample_t>;
    using exploration_constant_t  = monte_carlo::uniform_exploration_constant<double>;
    using value_delta_t           = monte_carlo::uniform_value_delta<double>;
    using value_update_t          = monte_carlo::uniform_value_update<mcts_tree_node_id, value_table_t, value_table_t, value_delta_t>;
    using cursor_t                = monte_carlo::uct_cursor<mcts_tree_node_id>;
    using path_t                  = monte_carlo::uct_backprop_path<mcts_tree_node_id>;
    using visit_creditor_t        = monte_carlo::uct_visit_creditor<mcts_tree_node_id, path_t, visits_table_t, visits_table_t>;
    using value_creditor_t        = monte_carlo::uct_value_creditor<visit_creditor_t, path_t, value_update_t>;
    using goal_policy_t           = monte_carlo::ucb1<mcts_tree_node_id, mcts_choice, double,
                                         visits_table_t, value_table_t, tree_walker,
                                         exploration_constant_t, choices_t, choices_t>;
    using policy_t                = phoenix_policy_choose<check_mcts_choice_is_rule_choice, goal_policy_t, rule_policy_t>;
    using chooser_t               = monte_carlo::uct_chooser<mcts_tree_node_id, mcts_choice, visits_table_t, tree_walker,
                                                choices_t, choices_t,
                                                policy_t, rollout_t,
                                                cursor_t, cursor_t, path_t,
                                                monte_carlo::in_rollout_flag, monte_carlo::in_rollout_flag>;
    using terminator_t            = monte_carlo::uct_terminator<mcts_tree_node_id,
                                                   path_t,
                                                   value_creditor_t,
                                                   path_t, path_t,
                                                   cursor_t,
                                                   monte_carlo::in_rollout_flag>;

    phoenix_fc_uct_manifest(visits_table_t&, visits_table_t&, value_table_t&, value_table_t&,
                            tree_walker&, rollout_t&, exploration_constant_t&, value_delta_t&,
                            std::mt19937&, mcts_tree_node_id);

    tree_walker&                     walker;
    rollout_t&                       rollout;
    index_sample_t                   index_sample;
    rule_policy_t                    rule_policy;
    exploration_constant_t&          exploration_constant;
    value_delta_t&                   delta;
    value_update_t                   value_update;
    cursor_t                         cursor;
    path_t                           backprop_path;
    visit_creditor_t                 visit_creditor;
    value_creditor_t                 value_creditor;
    goal_policy_t                    goal_policy;
    check_mcts_choice_is_rule_choice check_is_rule_choice;
    policy_t                         policy;
    monte_carlo::in_rollout_flag     in_rollout;
    chooser_t                        chooser;
    terminator_t                     terminator;
};

#endif
