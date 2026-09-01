#ifndef PHOENIX_DBUCT_MANIFEST_HPP
#define PHOENIX_DBUCT_MANIFEST_HPP

#include <cstddef>
#include <limits>
#include <random>
#include <unordered_map>
#include <vector>
#include "infrastructure/check_mcts_choice_is_rule_choice.hpp"
#include "infrastructure/dbuct_chooser.hpp"
#include "infrastructure/dbuct_frame_stack.hpp"
#include "infrastructure/dbuct_frame_stack_controller.hpp"
#include "infrastructure/dbuct_terminator.hpp"
#include "infrastructure/dbuct_value_adder.hpp"
#include "infrastructure/dbuct_value_creditor.hpp"
#include "infrastructure/dbuct_value_stack.hpp"
#include "infrastructure/dbuct_value_stack_controller.hpp"
#include "infrastructure/dbuct_visit_adder.hpp"
#include "infrastructure/dbuct_visit_creditor.hpp"
#include "infrastructure/in_rollout_flag.hpp"
#include "infrastructure/phoenix_policy_choose.hpp"
#include "infrastructure/random_policy_choose.hpp"
#include "infrastructure/random_rollout.hpp"
#include "infrastructure/tree_walker.hpp"
#include "infrastructure/ucb1.hpp"
#include "infrastructure/uniform_exploration_constant.hpp"
#include "infrastructure/uniform_index_sample.hpp"
#include "infrastructure/uniform_value_delta.hpp"
#include "infrastructure/value_table.hpp"
#include "infrastructure/visit_proportional_grant.hpp"
#include "infrastructure/visits_table.hpp"
#include "value_objects/dbuct_frame.hpp"
#include "value_objects/dbuct_value_frame.hpp"
#include "value_objects/mcts_choice.hpp"
#include "value_objects/mcts_tree_node_id.hpp"

struct phoenix_dbuct_manifest {
    using choices_t                  = std::vector<mcts_choice>;
    using visits_table_t             = monte_carlo::visits_table<mcts_tree_node_id, std::unordered_map>;
    using value_table_t              = monte_carlo::value_table<mcts_tree_node_id, double, std::unordered_map>;
    using rollout_t                  = monte_carlo::random_rollout<mcts_choice, std::mt19937, choices_t, choices_t>;
    using index_sample_t             = uniform_index_sample<std::mt19937>;
    using rule_policy_t              = random_policy_choose<mcts_choice, index_sample_t>;
    using exploration_t              = monte_carlo::uniform_exploration_constant<double>;
    using delta_t                    = monte_carlo::uniform_value_delta<double>;
    using grant_t                    = monte_carlo::visit_proportional_grant<mcts_tree_node_id, double, visits_table_t>;
    using frame_stack_t              = monte_carlo::dbuct_frame_stack<mcts_tree_node_id>;
    using visit_adder_t              = monte_carlo::dbuct_visit_adder<mcts_tree_node_id, frame_stack_t,
                                                         visits_table_t, visits_table_t>;
    using frame_stack_controller_t   = monte_carlo::dbuct_frame_stack_controller<mcts_tree_node_id,
                                                                    frame_stack_t, frame_stack_t,
                                                                    frame_stack_t, visit_adder_t>;
    using value_stack_t              = monte_carlo::dbuct_value_stack<mcts_tree_node_id, double>;
    using value_adder_t              = monte_carlo::dbuct_value_adder<mcts_tree_node_id, double, value_stack_t,
                                                         value_table_t, value_table_t>;
    using value_stack_controller_t   = monte_carlo::dbuct_value_stack_controller<mcts_tree_node_id, double,
                                                                    frame_stack_controller_t,
                                                                    frame_stack_controller_t,
                                                                    value_stack_t, value_stack_t,
                                                                    value_stack_t, value_adder_t>;
    using visit_creditor_t           = monte_carlo::dbuct_visit_creditor<visit_adder_t>;
    using value_creditor_t           = monte_carlo::dbuct_value_creditor<visit_creditor_t, value_stack_t,
                                                             value_adder_t, delta_t>;
    using goal_policy_t              = monte_carlo::ucb1<mcts_tree_node_id, mcts_choice, double,
                                            visits_table_t, value_table_t, tree_walker,
                                            exploration_t, choices_t, choices_t>;
    using policy_t                   = phoenix_policy_choose<check_mcts_choice_is_rule_choice, goal_policy_t, rule_policy_t>;
    using chooser_t                  = monte_carlo::dbuct_chooser<mcts_tree_node_id, mcts_choice,
                                                     visits_table_t, grant_t,
                                                     value_stack_controller_t,
                                                     frame_stack_t,
                                                     tree_walker,
                                                     choices_t, choices_t,
                                                     policy_t, rollout_t,
                                                     monte_carlo::in_rollout_flag, monte_carlo::in_rollout_flag>;
    using terminator_t               = monte_carlo::dbuct_terminator<value_stack_controller_t,
                                                        frame_stack_t,
                                                        value_creditor_t,
                                                        monte_carlo::in_rollout_flag>;

    phoenix_dbuct_manifest(visits_table_t&, visits_table_t&, value_table_t&, value_table_t&,
                           std::mt19937&, double, double, mcts_tree_node_id);

    tree_walker                      walker;
    rollout_t                        rollout;
    index_sample_t                   index_sample;
    rule_policy_t                    rule_policy;
    exploration_t                    exploration_constant;
    delta_t                          delta;
    grant_t                          grant;
    frame_stack_t                    frame_stack;
    visit_adder_t                    visit_adder;
    frame_stack_controller_t         frame_stack_controller;
    value_stack_t                    value_stack;
    value_adder_t                    value_adder;
    value_stack_controller_t         value_stack_controller;
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
