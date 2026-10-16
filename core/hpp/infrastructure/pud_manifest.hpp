#ifndef PUD_MANIFEST_HPP
#define PUD_MANIFEST_HPP

#include <cstdint>
#include <vector>

#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/representative_change_logger.hpp"

#include "infrastructure/pud_acceptor.hpp"
#include "infrastructure/pud_axiom_initializer.hpp"
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
#include "infrastructure/pud_leaf_expander.hpp"
#include "infrastructure/pud_leaf_invalidator.hpp"
#include "infrastructure/pud_leaf_nodes.hpp"
#include "infrastructure/pud_live_leaf_finder.hpp"
#include "infrastructure/pud_lvc_descent.hpp"
#include "infrastructure/pud_mhcs.hpp"
#include "infrastructure/pud_mhq.hpp"
#include "infrastructure/pud_mhws.hpp"
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
#include "infrastructure/pud_open_body_goal_finder.hpp"
#include "infrastructure/pud_query_head.hpp"
#include "infrastructure/pud_query_head_factory.hpp"
#include "infrastructure/pud_query_head_forker.hpp"
#include "infrastructure/pud_query_head_nodes.hpp"
#include "infrastructure/pud_query_opener.hpp"
#include "infrastructure/pud_refutation_propagator.hpp"
#include "infrastructure/pud_refuted_leaves.hpp"
#include "infrastructure/pud_refuted_node_checker.hpp"
#include "infrastructure/pud_replay_descent.hpp"
#include "infrastructure/pud_root_descent_maker.hpp"
#include "infrastructure/pud_root_nodes.hpp"
#include "infrastructure/pud_solver.hpp"
#include "infrastructure/pud_structure_watch_descent.hpp"
#include "infrastructure/pud_transient_ubs_state.hpp"
#include "infrastructure/pud_translating_normalizer.hpp"
#include "infrastructure/pud_unifier.hpp"
#include "infrastructure/pud_unifier_factory.hpp"
#include "infrastructure/pud_witness_search_head.hpp"
#include "infrastructure/pud_witness_search_head_factory.hpp"
#include "infrastructure/pud_witness_search_head_forker.hpp"

struct pud_manifest {
    using child_iter_t = std::vector<pud_node_id>::const_iterator;

    using bind_map_t = ubs_transient_bind_map<expr_pool, expr_pool>;
    using normalizer_factory_t = pud_normalizer_factory<globalizer, expr_pool, bind_map_t>;
    using normalizer_t = normalizer_factory_t::normalizer_t;
    using unifier_factory_t = pud_unifier_factory<globalizer, normalizer_t, bind_map_t>;
    using unifier_t = unifier_factory_t::unifier_t;
    using logger_t = representative_change_logger<bind_map_t, bind_map_t, std::vector<uint32_t>>;
    using logging_unifier_t = pud_unifier<globalizer, normalizer_t, bind_map_t, logger_t, logger_t>;
    using translating_normalizer_t = pud_translating_normalizer<globalizer, expr_pool, expr_pool, bind_map_t>;

    using refuted_node_checker_t = pud_refuted_node_checker<
        pud_leaf_nodes, pud_refuted_leaves, pud_node_live_child_counts>;

    using replay_descent_t = pud_replay_descent<
        bind_map_t, normalizer_t, unifier_t, expr_pool, expr_pool,
        normalizer_factory_t, unifier_factory_t, pud_node_added_substitutions>;

    using witness_search_head_t = pud_witness_search_head<
        replay_descent_t, child_iter_t, pud_leaf_nodes, refuted_node_checker_t, pud_children,
        pud_call_sites>;
    using witness_search_head_factory_t = pud_witness_search_head_factory<
        replay_descent_t, child_iter_t, pud_leaf_nodes, refuted_node_checker_t, pud_children,
        pud_call_sites>;
    using witness_search_head_forker_t = pud_witness_search_head_forker<
        replay_descent_t, child_iter_t, pud_leaf_nodes, refuted_node_checker_t, pud_children,
        pud_call_sites>;
    using mhws_t = pud_mhws<
        replay_descent_t, child_iter_t, witness_search_head_t,
        witness_search_head_factory_t, witness_search_head_forker_t>;

    using candidate_specialization_head_t = pud_candidate_specialization_head<
        replay_descent_t, child_iter_t, mhws_t, mhws_t, mhws_t,
        pud_leaf_nodes, refuted_node_checker_t, pud_children>;
    using candidate_specialization_head_factory_t = pud_candidate_specialization_head_factory<
        replay_descent_t, child_iter_t, mhws_t, mhws_t, mhws_t,
        pud_leaf_nodes, refuted_node_checker_t, pud_children>;
    using candidate_specialization_head_forker_t = pud_candidate_specialization_head_forker<
        replay_descent_t, child_iter_t, mhws_t, mhws_t, mhws_t,
        pud_leaf_nodes, refuted_node_checker_t, pud_children>;
    using mhcs_t = pud_mhcs<
        replay_descent_t, candidate_specialization_head_t,
        candidate_specialization_head_factory_t, candidate_specialization_head_forker_t, mhws_t>;

    using query_head_t = pud_query_head<replay_descent_t, mhcs_t, mhcs_t>;
    using query_head_factory_t = pud_query_head_factory<replay_descent_t, mhcs_t, mhcs_t>;
    using query_head_forker_t = pud_query_head_forker<replay_descent_t, mhcs_t, mhcs_t>;
    using mhq_t = pud_mhq<replay_descent_t, query_head_t, query_head_factory_t, query_head_forker_t, mhcs_t>;

    using query_opener_t = pud_query_opener<
        replay_descent_t, bind_map_t, normalizer_t, unifier_t, pud_root_nodes, pud_node_heads,
        expr_pool, expr_pool, normalizer_factory_t, unifier_factory_t, pud_node_added_substitutions>;

    using binding_descent_t = pud_binding_descent<bind_map_t, pud_node_added_substitutions, expr_pool, expr_pool>;
    using lvc_descent_t = pud_lvc_descent<pud_node_added_var_count>;
    using next_body_goal_id_descent_t = pud_next_body_goal_id_descent<pud_node_added_body_goals>;
    using body_goal_expr_descent_t = pud_body_goal_expr_descent<pud_node_added_body_goals, pud_call_sites>;
    using structure_watch_descent_t = pud_structure_watch_descent<pud_node_added_substitutions>;
    using body_goal_pq_descent_t = pud_body_goal_pq_descent<
        query_opener_t, mhq_t, mhq_t, mhq_t, pud_call_sites, pud_query_head_nodes>;
    using base_descent_t = pud_base_descent<
        binding_descent_t, lvc_descent_t, next_body_goal_id_descent_t, body_goal_expr_descent_t,
        structure_watch_descent_t, body_goal_pq_descent_t>;
    using node_descents_t = pud_node_descents<base_descent_t>;
    using root_descent_maker_t = pud_root_descent_maker<
        base_descent_t, binding_descent_t, lvc_descent_t, next_body_goal_id_descent_t,
        body_goal_expr_descent_t, structure_watch_descent_t, body_goal_pq_descent_t,
        pud_node_added_substitutions, expr_pool, expr_pool, pud_node_added_var_count,
        pud_node_added_body_goals, pud_call_sites, query_opener_t, mhq_t, mhq_t, mhq_t,
        pud_query_head_nodes>;

    using open_body_goal_finder_t = pud_open_body_goal_finder<pud_node_added_body_goals, pud_call_sites>;
    using acceptor_t = pud_acceptor<
        bind_map_t, normalizer_t, logging_unifier_t, logger_t, translating_normalizer_t,
        open_body_goal_finder_t, pud_node_heads, pud_node_added_substitutions,
        globalizer, expr_pool, expr_pool, normalizer_factory_t>;
    using child_maker_t = pud_child_maker<
        pud_node_id_sequencer, pud_node_added_substitutions, pud_node_added_body_goals,
        pud_node_added_var_count, pud_node_parents, pud_leaf_nodes>;

    using live_leaf_finder_t = pud_live_leaf_finder<refuted_node_checker_t, pud_leaf_nodes, pud_children>;
    using refutation_propagator_t = pud_refutation_propagator<
        pud_root_nodes, pud_node_parents, pud_node_live_child_counts>;
    using leaf_invalidator_t = pud_leaf_invalidator<mhws_t, mhcs_t, mhcs_t, mhq_t>;
    using node_refuter_t = pud_node_refuter<
        live_leaf_finder_t, pud_refuted_leaves, refutation_propagator_t, node_descents_t,
        leaf_invalidator_t, pud_query_head_nodes>;

    using leaf_expander_t = pud_leaf_expander<
        node_descents_t, mhq_t, mhcs_t, acceptor_t, child_maker_t, pud_call_sites, pud_children,
        pud_leaf_nodes, pud_node_live_child_counts, refutation_propagator_t, leaf_invalidator_t,
        pud_query_head_nodes, node_refuter_t, refuted_node_checker_t, node_descents_t,
        node_descents_t>;
    using solver_t = pud_solver<
        live_leaf_finder_t, node_descents_t, leaf_expander_t, root_descent_maker_t,
        node_descents_t, node_refuter_t>;
    using axiom_initializer_t = pud_axiom_initializer<
        pud_node_id_sequencer, pud_node_heads, pud_node_added_body_goals, pud_node_added_var_count,
        pud_root_nodes, pud_leaf_nodes>;

    pud_manifest();

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
    pud_leaf_nodes                          leaf_nodes_;
    pud_refuted_leaves                      refuted_leaves_;
    pud_node_live_child_counts              node_live_child_counts_;
    pud_query_head_nodes                    query_head_nodes_;
    refuted_node_checker_t                  refuted_node_checker_;

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

    leaf_expander_t                         leaf_expander_;
    solver_t                                solver_;
    axiom_initializer_t                     axiom_initializer_;
};

#endif
