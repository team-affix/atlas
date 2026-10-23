#ifndef PUD_MANIFEST_HPP
#define PUD_MANIFEST_HPP

#include <cstdint>
#include <deque>
#include <optional>
#include <vector>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_acceptor.hpp"
#include "infrastructure/pud_binder.hpp"
#include "infrastructure/pud_changed_class_finder.hpp"
#include "infrastructure/pud_expander.hpp"
#include "infrastructure/pud_frontier_change_collector.hpp"
#include "infrastructure/pud_child_activator.hpp"
#include "infrastructure/pud_leaf_refuter.hpp"
#include "infrastructure/pud_leaf_query_maker.hpp"
#include "infrastructure/pud_node_deactivator.hpp"
#include "infrastructure/pud_root_activator.hpp"
#include "infrastructure/pud_axiom_groups.hpp"
#include "infrastructure/pud_active_states.hpp"
#include "infrastructure/pud_mhcs.hpp"
#include "infrastructure/pud_mhq.hpp"
#include "infrastructure/pud_mhws.hpp"
#include "infrastructure/pud_node_goals.hpp"
#include "infrastructure/pud_node_id_sequencer.hpp"
#include "infrastructure/pud_node_live_child_counts.hpp"
#include "infrastructure/pud_node_maker.hpp"
#include "infrastructure/pud_leaf_nodes.hpp"
#include "infrastructure/pud_refuted_leaves.hpp"
#include "infrastructure/pud_refuted_node_checker.hpp"
#include "infrastructure/pud_node_table.hpp"
#include "infrastructure/pud_query_holders.hpp"
#include "infrastructure/pud_refutation_propagator.hpp"
#include "infrastructure/pud_replayer.hpp"
#include "infrastructure/pud_resolver.hpp"
#include "infrastructure/pud_root_opener.hpp"
#include "infrastructure/pud_structure_watcher.hpp"
#include "infrastructure/pud_term_factory.hpp"
#include "infrastructure/pud_unfolder.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_axiom_group_id.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_substitution.hpp"

// Composition root for the progressive unfolding database (open roots, expand leaves).
struct pud_manifest {
    using term_factory_t = pud_term_factory<globalizer, expr_pool, expr_pool>;
    using root_heads_t = pud_node_table<const expr*>;
    using node_parents_t = pud_node_table<std::optional<pud_node_id>>;
    using node_substitutions_t = pud_node_table<std::vector<pud_substitution>>;
    using node_lvcs_t = pud_node_table<uint32_t>;
    using call_sites_t = pud_node_table<pud_body_goal_id>;
    using children_t = pud_node_table<std::vector<pud_node_id>>;
    using unit_queries_t = std::deque<pud_mhq_head_id>;

    using refuted_node_checker_t =
        pud_refuted_node_checker<pud_leaf_nodes, pud_refuted_leaves, pud_node_live_child_counts>;
    using refutation_propagator_t = pud_refutation_propagator<node_parents_t, pud_node_live_child_counts>;
    using node_maker_t = pud_node_maker<
        pud_node_id_sequencer, root_heads_t, node_parents_t, node_substitutions_t, pud_node_goals, pud_node_goals,
        node_lvcs_t, node_lvcs_t, pud_leaf_nodes, pud_axiom_groups, pud_axiom_groups>;

    using replayer_t = pud_replayer<root_heads_t, node_substitutions_t, expr_pool, term_factory_t, term_factory_t, term_factory_t>;
    using mhws_t = pud_mhws<replayer_t, pud_leaf_nodes, refuted_node_checker_t, children_t>;
    using mhcs_t =
        pud_mhcs<replayer_t, mhws_t, mhws_t, mhws_t, mhws_t, mhws_t, pud_leaf_nodes, refuted_node_checker_t, children_t>;
    using mhq_t = pud_mhq<replayer_t, pud_axiom_groups, mhcs_t, mhcs_t, mhcs_t, mhcs_t, unit_queries_t>;

    using binder_t = pud_binder<term_factory_t, node_substitutions_t>;
    using structure_watcher_t = pud_structure_watcher<term_factory_t, node_substitutions_t>;
    using leaf_query_maker_t = pud_leaf_query_maker<mhq_t, mhq_t, pud_query_holders, pud_node_goals, node_lvcs_t>;
    using root_activator_t =
        pud_root_activator<pud_active_states, structure_watcher_t, pud_node_goals, leaf_query_maker_t>;
    using child_activator_t = pud_child_activator<
        pud_active_states, pud_active_states, binder_t, structure_watcher_t, pud_node_goals, call_sites_t,
        leaf_query_maker_t>;
    using node_deactivator_t = pud_node_deactivator<
        pud_active_states, pud_active_states, pud_query_holders, mhq_t, mhq_t>;
    using leaf_refuter_t = pud_leaf_refuter<pud_refuted_leaves, refutation_propagator_t>;
    using change_collector_t = pud_frontier_change_collector<refuted_node_checker_t>;

    using acceptor_t = pud_acceptor<pud_node_goals, replayer_t, replayer_t, expr_pool, term_factory_t, term_factory_t, pud_changed_class_finder>;
    using resolver_t = pud_resolver<pud_active_states, pud_node_goals, node_lvcs_t, mhq_t, mhcs_t, acceptor_t>;
    using unfolder_t =
        pud_unfolder<call_sites_t, node_maker_t, children_t, pud_leaf_nodes, pud_node_live_child_counts, refutation_propagator_t>;
    using expander_t = pud_expander<
        resolver_t, unfolder_t, child_activator_t, leaf_refuter_t, node_deactivator_t, pud_query_holders, change_collector_t>;
    using root_opener_t = pud_root_opener<
        node_maker_t, pud_axiom_groups, pud_axiom_groups, root_activator_t, leaf_refuter_t, node_deactivator_t,
        pud_query_holders, change_collector_t>;

    pud_manifest();

    expr_pool                  expr_pool_;
    globalizer                 globalizer_;
    term_factory_t             term_factory_;
    pud_changed_class_finder   changed_class_finder_;

    pud_node_id_sequencer      node_id_sequencer_;
    pud_leaf_nodes             leaf_nodes_;
    pud_refuted_leaves         refuted_leaves_;
    root_heads_t               root_heads_;
    node_parents_t             node_parents_;
    node_substitutions_t       node_substitutions_;
    pud_node_goals             node_goals_;
    node_lvcs_t                node_lvcs_;
    call_sites_t               call_sites_;
    children_t                 children_;
    pud_node_live_child_counts live_child_counts_;
    refuted_node_checker_t     refuted_node_checker_;
    pud_axiom_groups           axiom_groups_;
    pud_axiom_group_id         database_axiom_group_;
    refutation_propagator_t    refutation_propagator_;
    node_maker_t               node_maker_;

    unit_queries_t             unit_queries_;
    replayer_t                 replayer_;
    mhws_t                     mhws_;
    mhcs_t                     mhcs_;
    mhq_t                      mhq_;

    pud_active_states          active_states_;
    pud_query_holders          query_holders_;
    binder_t                   binder_;
    structure_watcher_t        structure_watcher_;
    leaf_query_maker_t         leaf_query_maker_;
    root_activator_t           root_activator_;
    child_activator_t          child_activator_;
    node_deactivator_t         node_deactivator_;
    leaf_refuter_t             leaf_refuter_;
    change_collector_t         change_collector_;

    acceptor_t                 acceptor_;
    resolver_t                 resolver_;
    unfolder_t                 unfolder_;
    expander_t                 expander_;
    root_opener_t              root_opener_;
};

#endif
