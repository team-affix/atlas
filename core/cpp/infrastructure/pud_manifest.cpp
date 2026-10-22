#include "infrastructure/pud_manifest.hpp"

pud_manifest::pud_manifest()
    : expr_pool_(),
      globalizer_(),
      node_id_sequencer_(),
      node_heads_(),
      node_added_body_goals_(),
      node_added_var_count_(),
      node_added_substitutions_(),
      call_sites_(),
      children_(),
      node_parents_(),
      root_nodes_(),
      axioms_(),
      leaf_nodes_(),
      open_leaves_(),
      refuted_leaves_(),
      node_live_child_counts_(),
      query_head_nodes_(),
      query_forks_(),
      refuted_node_checker_(leaf_nodes_, refuted_leaves_, node_live_child_counts_),
      frontier_recorder_(open_leaves_, open_leaves_, refuted_leaves_),
      normalizer_factory_(globalizer_, expr_pool_, expr_pool_),
      cycle_detector_factory_(globalizer_),
      unifier_factory_(globalizer_),
      witness_search_head_factory_(leaf_nodes_, refuted_node_checker_, children_),
      witness_search_head_forker_(),
      mhws_(witness_search_head_factory_, witness_search_head_forker_),
      candidate_specialization_head_factory_(
          mhws_, mhws_, mhws_, mhws_, leaf_nodes_, refuted_node_checker_, children_),
      candidate_specialization_head_forker_(),
      mhcs_(candidate_specialization_head_factory_, candidate_specialization_head_forker_, mhws_),
      query_head_factory_(mhcs_, mhcs_),
      query_head_forker_(),
      mhq_(query_head_factory_, query_head_forker_, mhcs_),
      query_opener_(
          axioms_, node_heads_, expr_pool_, cycle_detector_factory_, unifier_factory_,
          node_added_substitutions_),
      node_descents_(),
      root_descent_maker_(
          node_added_substitutions_, node_added_var_count_,
          node_added_body_goals_, call_sites_, query_opener_, mhq_, mhq_,
          mhq_, query_head_nodes_,
          query_forks_),
      open_body_goal_finder_(node_added_body_goals_, call_sites_),
      acceptor_(
          open_body_goal_finder_, node_heads_, node_added_substitutions_,
          globalizer_, expr_pool_, expr_pool_, cycle_detector_factory_),
      child_maker_(
          node_id_sequencer_, node_added_substitutions_, node_added_body_goals_,
          node_added_var_count_, node_parents_, leaf_nodes_),
      live_leaf_finder_(refuted_node_checker_, leaf_nodes_, children_),
      refutation_propagator_(root_nodes_, node_parents_, node_live_child_counts_),
      leaf_invalidator_(mhws_, mhcs_, mhcs_, mhq_),
      node_refuter_(
          live_leaf_finder_, frontier_recorder_, open_leaves_, frontier_recorder_, refutation_propagator_,
          node_descents_, leaf_invalidator_, query_head_nodes_),
      unfolder_(
          node_descents_, mhq_, mhcs_, acceptor_, child_maker_, call_sites_, children_,
          leaf_nodes_, frontier_recorder_, node_live_child_counts_, refutation_propagator_,
          leaf_invalidator_, query_head_nodes_, node_refuter_, refuted_node_checker_,
          node_descents_, frontier_recorder_, node_descents_),
      leaf_activator_(node_descents_, frontier_recorder_, node_refuter_),
      base_descent_runner_(
          refuted_node_checker_, root_descent_maker_, leaf_nodes_, children_,
          leaf_activator_, node_refuter_),
      axiom_initializer_(
          node_id_sequencer_, node_heads_, node_added_body_goals_, node_added_var_count_,
          root_nodes_, leaf_nodes_, axioms_),
      conjecture_initializer_(
          node_id_sequencer_, node_heads_, node_added_body_goals_, node_added_var_count_, root_nodes_,
          leaf_nodes_) {}
