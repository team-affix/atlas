#ifndef PUD_MANIFEST_HPP
#define PUD_MANIFEST_HPP

#include <cstdint>
#include <deque>
#include <optional>
#include <random>
#include <vector>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_acceptor.hpp"
#include "infrastructure/pud_binder.hpp"
#include "infrastructure/pud_changed_class_finder.hpp"
#include "infrastructure/pud_expander.hpp"
#include "infrastructure/pud_frontier_change_collector.hpp"
#include "infrastructure/pud_frontier_retirer.hpp"
#include "infrastructure/pud_leaf_activator.hpp"
#include "infrastructure/pud_leaf_materializer.hpp"
#include "infrastructure/pud_leaf_query_maker.hpp"
#include "infrastructure/pud_leaf_releaser.hpp"
#include "infrastructure/pud_leaf_states.hpp"
#include "infrastructure/pud_mhcs.hpp"
#include "infrastructure/pud_mhq.hpp"
#include "infrastructure/pud_mhws.hpp"
#include "infrastructure/pud_node_goals.hpp"
#include "infrastructure/pud_node_id_sequencer.hpp"
#include "infrastructure/pud_node_live_child_counts.hpp"
#include "infrastructure/pud_node_maker.hpp"
#include "infrastructure/pud_node_states.hpp"
#include "infrastructure/pud_node_status.hpp"
#include "infrastructure/pud_node_table.hpp"
#include "infrastructure/pud_query_holders.hpp"
#include "infrastructure/pud_random_expansion_chooser.hpp"
#include "infrastructure/pud_refutation_propagator.hpp"
#include "infrastructure/pud_replayer.hpp"
#include "infrastructure/pud_resolver.hpp"
#include "infrastructure/pud_root_opener.hpp"
#include "infrastructure/pud_solver.hpp"
#include "infrastructure/pud_structure_watcher.hpp"
#include "infrastructure/pud_term_factory.hpp"
#include "infrastructure/pud_unfolder.hpp"
#include "infrastructure/pud_unit_unfold_finder.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_substitution.hpp"

// The composition root of the progressive unfolding database and its solver.
struct pud_manifest {
    using term_factory_t = pud_term_factory<globalizer, expr_pool, expr_pool>;
    using root_heads_t = pud_node_table<const expr*>;
    using node_parents_t = pud_node_table<std::optional<pud_node_id>>;
    using node_substitutions_t = pud_node_table<std::vector<pud_substitution>>;
    using node_lvcs_t = pud_node_table<uint32_t>;
    using call_sites_t = pud_node_table<pud_body_goal_id>;
    using children_t = pud_node_table<std::vector<pud_node_id>>;
    using axioms_t = std::vector<pud_node_id>;
    using unit_queries_t = std::deque<pud_mhq_head_id>;

    using node_status_t = pud_node_status<pud_node_states, pud_node_live_child_counts>;
    using refutation_propagator_t = pud_refutation_propagator<node_parents_t, pud_node_live_child_counts>;
    using node_maker_t = pud_node_maker<
        pud_node_id_sequencer, root_heads_t, node_parents_t, node_substitutions_t, pud_node_goals, pud_node_goals,
        node_lvcs_t, node_lvcs_t, pud_node_states>;

    using replayer_t = pud_replayer<root_heads_t, node_substitutions_t, expr_pool, term_factory_t, term_factory_t, term_factory_t>;
    using mhws_t = pud_mhws<replayer_t, node_status_t, node_status_t, children_t>;
    using mhcs_t = pud_mhcs<replayer_t, mhws_t, mhws_t, mhws_t, mhws_t, mhws_t, node_status_t, node_status_t, children_t>;
    using mhq_t = pud_mhq<replayer_t, axioms_t, mhcs_t, mhcs_t, mhcs_t, mhcs_t, unit_queries_t>;

    using binder_t = pud_binder<term_factory_t, node_substitutions_t>;
    using structure_watcher_t = pud_structure_watcher<term_factory_t, node_substitutions_t>;
    using leaf_query_maker_t = pud_leaf_query_maker<mhq_t, mhq_t, pud_query_holders, pud_node_goals, node_lvcs_t>;
    using leaf_activator_t = pud_leaf_activator<
        pud_leaf_states, pud_leaf_states, binder_t, structure_watcher_t, pud_node_goals, call_sites_t, leaf_query_maker_t>;
    using leaf_releaser_t = pud_leaf_releaser<pud_leaf_states, pud_leaf_states, pud_query_holders, mhq_t>;
    using frontier_retirer_t = pud_frontier_retirer<pud_node_states, refutation_propagator_t, leaf_releaser_t, mhq_t, pud_query_holders>;
    using change_collector_t = pud_frontier_change_collector<node_status_t, pud_node_goals>;

    using acceptor_t = pud_acceptor<pud_node_goals, replayer_t, replayer_t, expr_pool, term_factory_t, term_factory_t, pud_changed_class_finder>;
    using resolver_t = pud_resolver<pud_leaf_states, pud_node_goals, node_lvcs_t, mhq_t, mhcs_t, acceptor_t>;
    using unfolder_t = pud_unfolder<call_sites_t, node_maker_t, children_t, pud_node_states, pud_node_live_child_counts, refutation_propagator_t>;
    using expander_t = pud_expander<resolver_t, unfolder_t, leaf_activator_t, frontier_retirer_t, change_collector_t>;
    using root_opener_t = pud_root_opener<node_maker_t, axioms_t, leaf_activator_t, frontier_retirer_t, change_collector_t>;

    using unit_unfold_finder_t = pud_unit_unfold_finder<unit_queries_t, unit_queries_t, unit_queries_t, pud_query_holders>;
    using expansion_chooser_t = pud_random_expansion_chooser<pud_node_goals>;
    using leaf_materializer_t = pud_leaf_materializer<pud_leaf_states, term_factory_t, term_factory_t>;
    using solver_t = pud_solver<root_opener_t, root_opener_t, expander_t, unit_unfold_finder_t, expansion_chooser_t, leaf_materializer_t>;

    pud_manifest(uint32_t random_seed);

    expr_pool                  expr_pool_;
    globalizer                 globalizer_;
    term_factory_t             term_factory_;
    pud_changed_class_finder   changed_class_finder_;

    pud_node_id_sequencer      node_id_sequencer_;
    pud_node_states            node_states_;
    root_heads_t               root_heads_;
    node_parents_t             node_parents_;
    node_substitutions_t       node_substitutions_;
    pud_node_goals             node_goals_;
    node_lvcs_t                node_lvcs_;
    call_sites_t               call_sites_;
    children_t                 children_;
    pud_node_live_child_counts live_child_counts_;
    axioms_t                   axioms_;
    node_status_t              node_status_;
    refutation_propagator_t    refutation_propagator_;
    node_maker_t               node_maker_;

    unit_queries_t             unit_queries_;
    replayer_t                 replayer_;
    mhws_t                     mhws_;
    mhcs_t                     mhcs_;
    mhq_t                      mhq_;

    pud_leaf_states            leaf_states_;
    pud_query_holders          query_holders_;
    binder_t                   binder_;
    structure_watcher_t        structure_watcher_;
    leaf_query_maker_t         leaf_query_maker_;
    leaf_activator_t           leaf_activator_;
    leaf_releaser_t            leaf_releaser_;
    frontier_retirer_t         frontier_retirer_;
    change_collector_t         change_collector_;

    acceptor_t                 acceptor_;
    resolver_t                 resolver_;
    unfolder_t                 unfolder_;
    expander_t                 expander_;
    root_opener_t              root_opener_;

    std::mt19937               rng_;
    unit_unfold_finder_t       unit_unfold_finder_;
    expansion_chooser_t        expansion_chooser_;
    leaf_materializer_t        leaf_materializer_;
    solver_t                   solver_;
};

#endif
