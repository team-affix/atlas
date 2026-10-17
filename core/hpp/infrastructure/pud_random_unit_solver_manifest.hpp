#ifndef PUD_RANDOM_UNIT_SOLVER_MANIFEST_HPP
#define PUD_RANDOM_UNIT_SOLVER_MANIFEST_HPP

#include <cstdint>
#include <random>
#include <vector>

#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/representative_change_logger.hpp"

#include "infrastructure/pud_acceptor.hpp"
#include "infrastructure/pud_axiom_initializer.hpp"
#include "infrastructure/pud_axioms.hpp"
#include "infrastructure/pud_base_descent_runner.hpp"
#include "infrastructure/pud_base_descent.hpp"
#include "infrastructure/pud_binding_descent.hpp"
#include "infrastructure/pud_body_goal_expr_descent.hpp"
#include "infrastructure/pud_body_goal_pq_descent.hpp"
#include "infrastructure/pud_call_sites.hpp"
#include "infrastructure/pud_candidate_specialization_head.hpp"
#include "infrastructure/pud_candidate_specialization_head_factory.hpp"
#include "infrastructure/pud_candidate_specialization_head_forker.hpp"
#include "infrastructure/pud_child_maker.hpp"
#include "infrastructure/pud_children.hpp"
#include "infrastructure/pud_conjecture_initializer.hpp"
#include "infrastructure/pud_decision_generator.hpp"
#include "infrastructure/pud_leaf_activator.hpp"
#include "infrastructure/pud_unfolder.hpp"
#include "infrastructure/pud_leaf_invalidator.hpp"
#include "infrastructure/pud_leaf_nodes.hpp"
#include "infrastructure/pud_live_leaf_finder.hpp"
#include "infrastructure/pud_lvc_descent.hpp"
#include "infrastructure/pud_mhcs.hpp"
#include "infrastructure/pud_mhq.hpp"
#include "infrastructure/pud_mhws.hpp"
#include "infrastructure/pud_next_unfold.hpp"
#include "infrastructure/pud_next_unit_unfold.hpp"
#include "infrastructure/pud_next_body_goal_id_descent.hpp"
#include "infrastructure/pud_node_added_body_goals.hpp"
#include "infrastructure/pud_node_added_substitutions.hpp"
#include "infrastructure/pud_node_added_var_count.hpp"
#include "infrastructure/pud_node_descents.hpp"
#include "infrastructure/pud_node_heads.hpp"
#include "infrastructure/pud_node_id_sequencer.hpp"
#include "infrastructure/pud_node_live_child_counts.hpp"
#include "infrastructure/pud_node_parents.hpp"
#include "infrastructure/pud_node_refuter.hpp"
#include "infrastructure/pud_normalizer_factory.hpp"
#include "infrastructure/pud_open_conjecture_leaves.hpp"
#include "infrastructure/pud_open_leaves.hpp"
#include "infrastructure/pud_open_body_goal_finder.hpp"
#include "infrastructure/pud_query_head.hpp"
#include "infrastructure/pud_query_head_factory.hpp"
#include "infrastructure/pud_query_head_forker.hpp"
#include "infrastructure/pud_query_forks.hpp"
#include "infrastructure/pud_query_head_nodes.hpp"
#include "infrastructure/pud_query_opener.hpp"
#include "infrastructure/pud_random_rule_chooser.hpp"
#include "infrastructure/pud_random_subgoal_chooser.hpp"
#include "infrastructure/pud_refutation_propagator.hpp"
#include "infrastructure/pud_refuted_leaves.hpp"
#include "infrastructure/pud_refuted_node_checker.hpp"
#include "infrastructure/pud_replay_descent.hpp"
#include "infrastructure/pud_root_descent_maker.hpp"
#include "infrastructure/pud_root_nodes.hpp"
#include "infrastructure/pud_structure_watch_descent.hpp"
#include "infrastructure/pud_transient_ubs_state.hpp"
#include "infrastructure/pud_translating_normalizer.hpp"
#include "infrastructure/pud_unifier.hpp"
#include "infrastructure/pud_unfold_loop.hpp"
#include "infrastructure/pud_unifier_factory.hpp"
#include "infrastructure/pud_unit_queries.hpp"
#include "infrastructure/pud_unit_query_recorder.hpp"
#include "infrastructure/pud_witness_search_head.hpp"
#include "infrastructure/pud_witness_search_head_factory.hpp"
#include "infrastructure/pud_witness_search_head_forker.hpp"

// pud_random_solver_manifest with unit propagation: every query that becomes unit is queued, and the
// unfold loop forces the leaves holding queued unit queries, anywhere in the database, before deciding
// among the open conjecture leaves.
struct pud_random_unit_solver_manifest {
    using child_iter_t = std::vector<pud_node_id>::const_iterator;

    using bind_map_t = ubs_bind_map<
        expr_pool, expr_pool, pud_transient_ubs_state, pud_transient_ubs_state, pud_transient_ubs_state,
        pud_transient_ubs_state, pud_transient_ubs_state, pud_transient_ubs_state>;
    struct normalizer_factory_t : pud_normalizer_factory<globalizer, expr_pool, bind_map_t> {
        using pud_normalizer_factory::pud_normalizer_factory;
    };
    using normalizer_t = normalizer_factory_t::normalizer_t;
    struct unifier_factory_t : pud_unifier_factory<globalizer, normalizer_t, bind_map_t> {
        using pud_unifier_factory::pud_unifier_factory;
    };
    using unifier_t = unifier_factory_t::unifier_t;
    using logger_t = representative_change_logger<bind_map_t, bind_map_t, std::vector<uint32_t>>;
    using logging_unifier_t = pud_unifier<globalizer, normalizer_t, bind_map_t, logger_t, logger_t>;
    using translating_normalizer_t = pud_translating_normalizer<globalizer, expr_pool, expr_pool, bind_map_t>;

    // Composed collaborators are named structs rather than aliases, so a type that takes one as a
    // template argument spells only its short name instead of nesting its full instantiation.
    struct refuted_node_checker_t : pud_refuted_node_checker<
        pud_leaf_nodes, pud_refuted_leaves, pud_node_live_child_counts> {
        using pud_refuted_node_checker::pud_refuted_node_checker;
    };

    using replay_descent_t = pud_replay_descent<
        bind_map_t, normalizer_t, unifier_t, expr_pool, expr_pool,
        normalizer_factory_t, unifier_factory_t, pud_node_added_substitutions>;

    struct witness_search_head_t : pud_witness_search_head<
        replay_descent_t, child_iter_t, pud_leaf_nodes, refuted_node_checker_t, pud_children> {
        using pud_witness_search_head::pud_witness_search_head;
    };
    struct witness_search_head_factory_t : pud_witness_search_head_factory<
        witness_search_head_t, replay_descent_t, pud_leaf_nodes, refuted_node_checker_t, pud_children> {
        using pud_witness_search_head_factory::pud_witness_search_head_factory;
    };
    struct witness_search_head_forker_t : pud_witness_search_head_forker<witness_search_head_t, replay_descent_t> {
        using pud_witness_search_head_forker::pud_witness_search_head_forker;
    };
    struct mhws_t : pud_mhws<
        replay_descent_t, child_iter_t, witness_search_head_t,
        witness_search_head_factory_t, witness_search_head_forker_t> {
        using pud_mhws::pud_mhws;
    };

    struct candidate_specialization_head_t : pud_candidate_specialization_head<
        replay_descent_t, child_iter_t, mhws_t, mhws_t, mhws_t, mhws_t,
        pud_leaf_nodes, refuted_node_checker_t, pud_children> {
        using pud_candidate_specialization_head::pud_candidate_specialization_head;
    };
    struct candidate_specialization_head_factory_t : pud_candidate_specialization_head_factory<
        candidate_specialization_head_t, replay_descent_t, mhws_t, mhws_t, mhws_t, mhws_t,
        pud_leaf_nodes, refuted_node_checker_t, pud_children> {
        using pud_candidate_specialization_head_factory::pud_candidate_specialization_head_factory;
    };
    struct candidate_specialization_head_forker_t : pud_candidate_specialization_head_forker<candidate_specialization_head_t, replay_descent_t> {
        using pud_candidate_specialization_head_forker::pud_candidate_specialization_head_forker;
    };
    struct mhcs_t : pud_mhcs<
        replay_descent_t, candidate_specialization_head_t,
        candidate_specialization_head_factory_t, candidate_specialization_head_forker_t, mhws_t> {
        using pud_mhcs::pud_mhcs;
    };

    struct query_head_t : pud_query_head<replay_descent_t, mhcs_t, mhcs_t> {
        using pud_query_head::pud_query_head;
    };
    struct query_head_factory_t : pud_query_head_factory<query_head_t, replay_descent_t, mhcs_t, mhcs_t> {
        using pud_query_head_factory::pud_query_head_factory;
    };
    struct query_head_forker_t : pud_query_head_forker<query_head_t, replay_descent_t> {
        using pud_query_head_forker::pud_query_head_forker;
    };
    struct mhq_t : pud_mhq<replay_descent_t, query_head_t, query_head_factory_t, query_head_forker_t, mhcs_t> {
        using pud_mhq::pud_mhq;
    };
    struct unit_query_recorder_t : pud_unit_query_recorder<replay_descent_t, mhq_t, mhq_t, mhq_t, pud_unit_queries> {
        using pud_unit_query_recorder::pud_unit_query_recorder;
    };

    struct query_opener_t : pud_query_opener<
        replay_descent_t, bind_map_t, normalizer_t, unifier_t, pud_axioms, pud_node_heads,
        expr_pool, expr_pool, normalizer_factory_t, unifier_factory_t, pud_node_added_substitutions> {
        using pud_query_opener::pud_query_opener;
    };

    using binding_descent_t = pud_binding_descent<bind_map_t, pud_node_added_substitutions, expr_pool, expr_pool>;
    using lvc_descent_t = pud_lvc_descent<pud_node_added_var_count>;
    using next_body_goal_id_descent_t = pud_next_body_goal_id_descent<pud_node_added_body_goals>;
    using body_goal_expr_descent_t = pud_body_goal_expr_descent<pud_node_added_body_goals, pud_call_sites>;
    using structure_watch_descent_t = pud_structure_watch_descent<pud_node_added_substitutions>;
    using body_goal_pq_descent_t = pud_body_goal_pq_descent<
        query_opener_t, unit_query_recorder_t, unit_query_recorder_t, mhq_t, pud_call_sites,
        pud_query_head_nodes, pud_query_forks>;
    using base_descent_t = pud_base_descent<
        binding_descent_t, lvc_descent_t, next_body_goal_id_descent_t, body_goal_expr_descent_t,
        structure_watch_descent_t, body_goal_pq_descent_t>;
    struct node_descents_t : pud_node_descents<base_descent_t> {
        using pud_node_descents::pud_node_descents;
    };
    struct root_descent_maker_t : pud_root_descent_maker<
        base_descent_t, binding_descent_t, lvc_descent_t, next_body_goal_id_descent_t,
        body_goal_expr_descent_t, structure_watch_descent_t, body_goal_pq_descent_t,
        pud_node_added_substitutions, expr_pool, expr_pool, pud_node_added_var_count,
        pud_node_added_body_goals, pud_call_sites, query_opener_t, unit_query_recorder_t,
        unit_query_recorder_t, mhq_t, pud_query_head_nodes, pud_query_forks> {
        using pud_root_descent_maker::pud_root_descent_maker;
    };

    struct open_body_goal_finder_t : pud_open_body_goal_finder<pud_node_added_body_goals, pud_call_sites> {
        using pud_open_body_goal_finder::pud_open_body_goal_finder;
    };
    struct acceptor_t : pud_acceptor<
        bind_map_t, normalizer_t, logging_unifier_t, logger_t, translating_normalizer_t,
        open_body_goal_finder_t, pud_node_heads, pud_node_added_substitutions,
        globalizer, expr_pool, expr_pool, normalizer_factory_t> {
        using pud_acceptor::pud_acceptor;
    };
    struct child_maker_t : pud_child_maker<
        pud_node_id_sequencer, pud_node_added_substitutions, pud_node_added_body_goals,
        pud_node_added_var_count, pud_node_parents, pud_leaf_nodes> {
        using pud_child_maker::pud_child_maker;
    };

    struct live_leaf_finder_t : pud_live_leaf_finder<refuted_node_checker_t, pud_leaf_nodes, pud_children> {
        using pud_live_leaf_finder::pud_live_leaf_finder;
    };
    struct refutation_propagator_t : pud_refutation_propagator<
        pud_root_nodes, pud_node_parents, pud_node_live_child_counts> {
        using pud_refutation_propagator::pud_refutation_propagator;
    };
    struct leaf_invalidator_t : pud_leaf_invalidator<mhws_t, mhcs_t, mhcs_t, unit_query_recorder_t> {
        using pud_leaf_invalidator::pud_leaf_invalidator;
    };
    struct open_conjecture_leaves_t : pud_open_conjecture_leaves<
        pud_open_leaves, pud_open_leaves, pud_root_nodes, pud_axioms, pud_node_parents> {
        using pud_open_conjecture_leaves::pud_open_conjecture_leaves;
    };
    struct node_refuter_t : pud_node_refuter<
        live_leaf_finder_t, pud_refuted_leaves, pud_open_leaves, open_conjecture_leaves_t,
        refutation_propagator_t, node_descents_t, leaf_invalidator_t, pud_query_head_nodes> {
        using pud_node_refuter::pud_node_refuter;
    };

    struct unfolder_t : pud_unfolder<
        node_descents_t, mhq_t, mhcs_t, acceptor_t, child_maker_t, pud_call_sites, pud_children,
        pud_leaf_nodes, open_conjecture_leaves_t, pud_node_live_child_counts, refutation_propagator_t,
        leaf_invalidator_t, pud_query_head_nodes, node_refuter_t, refuted_node_checker_t,
        node_descents_t, open_conjecture_leaves_t, node_descents_t> {
        using pud_unfolder::pud_unfolder;
    };
    struct leaf_activator_t : pud_leaf_activator<base_descent_t, node_descents_t, open_conjecture_leaves_t, node_refuter_t> {
        using pud_leaf_activator::pud_leaf_activator;
    };
    struct base_descent_runner_t : pud_base_descent_runner<
        base_descent_t, refuted_node_checker_t, root_descent_maker_t, pud_leaf_nodes,
        pud_children, leaf_activator_t, node_refuter_t> {
        using pud_base_descent_runner::pud_base_descent_runner;
    };
    struct random_subgoal_chooser_t : pud_random_subgoal_chooser<node_descents_t> {
        using pud_random_subgoal_chooser::pud_random_subgoal_chooser;
    };
    struct decision_generator_t : pud_decision_generator<pud_random_rule_chooser, random_subgoal_chooser_t> {
        using pud_decision_generator::pud_decision_generator;
    };
    struct next_unit_unfold_t : pud_next_unit_unfold<
        pud_unit_queries, pud_unit_queries, pud_unit_queries, mhq_t, pud_query_head_nodes,
        refuted_node_checker_t, pud_query_forks, pud_leaf_nodes, pud_children, node_descents_t> {
        using pud_next_unit_unfold::pud_next_unit_unfold;
    };
    struct next_unfold_t : pud_next_unfold<next_unit_unfold_t, decision_generator_t> {
        using pud_next_unfold::pud_next_unfold;
    };
    struct unfold_loop_t : pud_unfold_loop<open_conjecture_leaves_t, next_unfold_t, unfolder_t> {
        using pud_unfold_loop::pud_unfold_loop;
    };
    struct axiom_initializer_t : pud_axiom_initializer<
        pud_node_id_sequencer, pud_node_heads, pud_node_added_body_goals, pud_node_added_var_count,
        pud_root_nodes, pud_leaf_nodes, pud_axioms> {
        using pud_axiom_initializer::pud_axiom_initializer;
    };
    struct conjecture_initializer_t : pud_conjecture_initializer<
        pud_node_id_sequencer, pud_node_added_body_goals, pud_node_added_var_count,
        pud_root_nodes, pud_leaf_nodes> {
        using pud_conjecture_initializer::pud_conjecture_initializer;
    };

    pud_random_unit_solver_manifest(uint32_t random_seed);

    expr_pool                               expr_pool_;
    globalizer                              globalizer_;

    pud_node_id_sequencer                   node_id_sequencer_;
    pud_node_heads                          node_heads_;
    pud_node_added_body_goals               node_added_body_goals_;
    pud_node_added_var_count                node_added_var_count_;
    pud_node_added_substitutions            node_added_substitutions_;
    pud_call_sites                          call_sites_;
    pud_children                            children_;
    pud_node_parents                        node_parents_;
    pud_root_nodes                          root_nodes_;
    pud_axioms                              axioms_;
    pud_leaf_nodes                          leaf_nodes_;
    pud_open_leaves                         open_leaves_;
    pud_refuted_leaves                      refuted_leaves_;
    pud_node_live_child_counts              node_live_child_counts_;
    pud_query_head_nodes                    query_head_nodes_;
    pud_query_forks                         query_forks_;
    pud_unit_queries                        unit_queries_;
    refuted_node_checker_t                  refuted_node_checker_;
    open_conjecture_leaves_t                open_conjecture_leaves_;

    normalizer_factory_t                    normalizer_factory_;
    unifier_factory_t                       unifier_factory_;

    witness_search_head_factory_t           witness_search_head_factory_;
    witness_search_head_forker_t            witness_search_head_forker_;
    mhws_t                                  mhws_;
    candidate_specialization_head_factory_t candidate_specialization_head_factory_;
    candidate_specialization_head_forker_t  candidate_specialization_head_forker_;
    mhcs_t                                  mhcs_;
    query_head_factory_t                    query_head_factory_;
    query_head_forker_t                     query_head_forker_;
    mhq_t                                   mhq_;
    unit_query_recorder_t                   unit_query_recorder_;
    query_opener_t                          query_opener_;

    node_descents_t                         node_descents_;
    root_descent_maker_t                    root_descent_maker_;

    open_body_goal_finder_t                 open_body_goal_finder_;
    acceptor_t                              acceptor_;
    child_maker_t                           child_maker_;

    live_leaf_finder_t                      live_leaf_finder_;
    refutation_propagator_t                 refutation_propagator_;
    leaf_invalidator_t                      leaf_invalidator_;
    node_refuter_t                          node_refuter_;

    unfolder_t                         unfolder_;
    leaf_activator_t                        leaf_activator_;
    base_descent_runner_t                   base_descent_runner_;
    std::mt19937                            rng_;
    pud_random_rule_chooser                 random_rule_chooser_;
    random_subgoal_chooser_t                random_subgoal_chooser_;
    decision_generator_t                    decision_generator_;
    next_unit_unfold_t                      next_unit_unfold_;
    next_unfold_t                           next_unfold_;
    unfold_loop_t                           unfold_loop_;
    axiom_initializer_t                     axiom_initializer_;
    conjecture_initializer_t                conjecture_initializer_;
};

#endif
