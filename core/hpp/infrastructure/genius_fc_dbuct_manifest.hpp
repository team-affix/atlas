#ifndef GENIUS_FC_DBUCT_MANIFEST_HPP
#define GENIUS_FC_DBUCT_MANIFEST_HPP

#include <cstddef>
#include <limits>
#include <map>
#include <random>
#include <vector>
#include "infrastructure/dbuct_chooser.hpp"
#include "infrastructure/dbuct_cumulative_grounded_weight.hpp"
#include "infrastructure/dbuct_decision_memory.hpp"
#include "infrastructure/dbuct_frame_stack.hpp"
#include "infrastructure/dbuct_frame_stack_controller.hpp"
#include "infrastructure/dbuct_rp_srt_active_goals.hpp"
#include "infrastructure/dbuct_srt_active_goals.hpp"
#include "infrastructure/dbuct_terminator.hpp"
#include "infrastructure/dbuct_value_adder.hpp"
#include "infrastructure/dbuct_value_creditor.hpp"
#include "infrastructure/dbuct_value_stack.hpp"
#include "infrastructure/dbuct_value_stack_controller.hpp"
#include "infrastructure/dbuct_visit_adder.hpp"
#include "infrastructure/dbuct_visit_creditor.hpp"
#include "infrastructure/genius_exploration_constant.hpp"
#include "infrastructure/genius_value_delta.hpp"
#include "infrastructure/horizon_reward.hpp"
#include "infrastructure/in_rollout_flag.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/ridge_reward.hpp"
#include "infrastructure/rp_fewer_candidate_goal_rollout.hpp"
#include "infrastructure/rp_heuristic_rollout.hpp"
#include "infrastructure/rp_uniform_rule_rollout.hpp"
#include "infrastructure/scope_walker.hpp"
#include "infrastructure/ucb1.hpp"
#include "infrastructure/value_table.hpp"
#include "infrastructure/visit_proportional_grant.hpp"
#include "infrastructure/visits_table.hpp"
#include "value_objects/dbuct_frame.hpp"
#include "value_objects/dbuct_value_frame.hpp"
#include "value_objects/mcts_choice.hpp"
#include "value_objects/mcts_scope_node_id.hpp"

struct genius_fc_dbuct_manifest {
    using choices_t                  = std::vector<mcts_choice>;
    using visits_table_t             = monte_carlo::visits_table<mcts_scope_node_id, std::map>;
    using value_table_t              = monte_carlo::value_table<mcts_scope_node_id, double, std::map>;
    using walker_t                   = scope_walker<lineage_pool>;
    using rp_srt_active_goals_t      = dbuct_rp_srt_active_goals<
                                            dbuct_srt_active_goals, dbuct_srt_active_goals,
                                            dbuct_srt_active_goals, dbuct_srt_active_goals>;
    using goal_rollout_t             = rp_fewer_candidate_goal_rollout<rp_srt_active_goals_t>;
    using rule_rollout_t             = rp_uniform_rule_rollout<std::mt19937>;
    using rollout_t                  = rp_heuristic_rollout<goal_rollout_t, rule_rollout_t>;
    using exploration_constant_t     = genius_exploration_constant<dbuct_srt_active_goals>;
    using value_delta_t              = genius_value_delta<ridge_reward<dbuct_decision_memory>,
                                             horizon_reward<dbuct_cumulative_grounded_weight>>;
    using grant_t                    = monte_carlo::visit_proportional_grant<mcts_scope_node_id, double, visits_table_t>;
    using frame_stack_t              = monte_carlo::dbuct_frame_stack<mcts_scope_node_id>;
    using visit_adder_t              = monte_carlo::dbuct_visit_adder<mcts_scope_node_id, frame_stack_t,
                                                         visits_table_t, visits_table_t>;
    using frame_stack_controller_t   = monte_carlo::dbuct_frame_stack_controller<mcts_scope_node_id,
                                                                    frame_stack_t, frame_stack_t,
                                                                    frame_stack_t, visit_adder_t>;
    using value_stack_t              = monte_carlo::dbuct_value_stack<mcts_scope_node_id, double>;
    using value_adder_t              = monte_carlo::dbuct_value_adder<mcts_scope_node_id, double, value_stack_t,
                                                         value_table_t, value_table_t>;
    using value_stack_controller_t   = monte_carlo::dbuct_value_stack_controller<mcts_scope_node_id, double,
                                                                    frame_stack_controller_t,
                                                                    frame_stack_controller_t,
                                                                    value_stack_t, value_stack_t,
                                                                    value_stack_t, value_adder_t>;
    using visit_creditor_t           = monte_carlo::dbuct_visit_creditor<visit_adder_t>;
    using value_creditor_t           = monte_carlo::dbuct_value_creditor<visit_creditor_t, value_stack_t,
                                                             value_adder_t, value_delta_t>;
    using policy_t                   = monte_carlo::ucb1<mcts_scope_node_id, mcts_choice, double,
                                            visits_table_t, value_table_t, walker_t,
                                            exploration_constant_t, choices_t, choices_t>;
    using chooser_t                  = monte_carlo::dbuct_chooser<mcts_scope_node_id, mcts_choice,
                                                     visits_table_t, grant_t,
                                                     value_stack_controller_t,
                                                     frame_stack_t,
                                                     walker_t,
                                                     choices_t, choices_t,
                                                     policy_t, rollout_t,
                                                     monte_carlo::in_rollout_flag, monte_carlo::in_rollout_flag>;
    using terminator_t               = monte_carlo::dbuct_terminator<value_stack_controller_t,
                                                        frame_stack_t,
                                                        value_creditor_t,
                                                        monte_carlo::in_rollout_flag>;

    genius_fc_dbuct_manifest(visits_table_t&, visits_table_t&, value_table_t&, value_table_t&,
                             walker_t&, rollout_t&, exploration_constant_t&, value_delta_t&,
                             double, mcts_scope_node_id);

    walker_t&                        walker;
    rollout_t&                       rollout;
    exploration_constant_t&          exploration_constant;
    value_delta_t&                   delta;
    grant_t                          grant;
    frame_stack_t                    frame_stack;
    visit_adder_t                    visit_adder;
    frame_stack_controller_t         frame_stack_controller;
    value_stack_t                    value_stack;
    value_adder_t                    value_adder;
    value_stack_controller_t         value_stack_controller;
    visit_creditor_t                 visit_creditor;
    value_creditor_t                 value_creditor;
    policy_t                         policy;
    monte_carlo::in_rollout_flag     in_rollout;
    chooser_t                        chooser;
    terminator_t                     terminator;
};

#endif
