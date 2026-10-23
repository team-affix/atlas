#ifndef PUD_MANIFEST_HPP
#define PUD_MANIFEST_HPP

#include <cstdint>
#include <deque>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_acceptor.hpp"
#include "infrastructure/pud_active_states.hpp"
#include "infrastructure/pud_axiom_groups.hpp"
#include "infrastructure/pud_binder.hpp"
#include "infrastructure/pud_child_activator.hpp"
#include "infrastructure/pud_data.hpp"
#include "infrastructure/pud_expander.hpp"
#include "infrastructure/pud_leaf_refuter.hpp"
#include "infrastructure/pud_mhcs.hpp"
#include "infrastructure/pud_mhq.hpp"
#include "infrastructure/pud_mhws.hpp"
#include "infrastructure/pud_node_deactivator.hpp"
#include "infrastructure/pud_node_goals.hpp"
#include "infrastructure/pud_node_live_child_counts.hpp"
#include "infrastructure/pud_node_maker.hpp"
#include "infrastructure/pud_node_table.hpp"
#include "infrastructure/pud_query_forker.hpp"
#include "infrastructure/pud_query_holders.hpp"
#include "infrastructure/pud_query_opener.hpp"
#include "infrastructure/pud_refutation_propagator.hpp"
#include "infrastructure/pud_refuted_leaves.hpp"
#include "infrastructure/pud_refuted_node_checker.hpp"
#include "infrastructure/pud_replayer.hpp"
#include "infrastructure/pud_resolver.hpp"
#include "infrastructure/pud_root_activator.hpp"
#include "infrastructure/pud_structure_watcher.hpp"
#include "infrastructure/pud_term_factory.hpp"
#include "value_objects/pud_axiom_group_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"

// Composition root for the progressive unfolding database (make and activate roots, expand leaves).
struct pud_manifest {
    using term_factory_t = pud_term_factory<globalizer, expr_pool, expr_pool>;
    using node_lvcs_t = pud_node_table<uint32_t>;
    using node_axiom_groups_t = pud_node_table<pud_axiom_group_id>;
    using unit_queries_t = std::deque<pud_mhq_head_id>;

    using refuted_node_checker_t = pud_refuted_node_checker<pud_data, pud_refuted_leaves, pud_node_live_child_counts>;
    using refutation_propagator_t = pud_refutation_propagator<pud_data, pud_node_live_child_counts>;
    using node_maker_t = pud_node_maker<
        pud_data, pud_data, pud_node_goals, pud_node_goals, node_lvcs_t, node_lvcs_t, node_axiom_groups_t,
        node_axiom_groups_t, pud_axiom_groups>;

    using replayer_t = pud_replayer<pud_data, pud_data, expr_pool, term_factory_t, term_factory_t, term_factory_t>;
    using mhws_t = pud_mhws<replayer_t, pud_data, refuted_node_checker_t, pud_data>;
    using mhcs_t = pud_mhcs<replayer_t, mhws_t, mhws_t, mhws_t, mhws_t, mhws_t, pud_data, refuted_node_checker_t, pud_data>;
    using mhq_t = pud_mhq<replayer_t, pud_axiom_groups, mhcs_t, mhcs_t, mhcs_t, mhcs_t, unit_queries_t>;

    using binder_t = pud_binder<term_factory_t, pud_data>;
    using structure_watcher_t = pud_structure_watcher<term_factory_t, pud_data>;
    using query_opener_t = pud_query_opener<mhq_t, pud_node_goals, node_lvcs_t, node_axiom_groups_t>;
    using query_forker_t = pud_query_forker<mhq_t, pud_node_goals, node_lvcs_t>;
    using root_activator_t =
        pud_root_activator<pud_active_states, structure_watcher_t, pud_node_goals, query_opener_t, pud_query_holders>;
    using child_activator_t = pud_child_activator<
        pud_active_states, pud_active_states, binder_t, structure_watcher_t, pud_node_goals, pud_data,
        query_forker_t, query_opener_t, pud_query_holders, mhq_t>;
    using node_deactivator_t = pud_node_deactivator<
        pud_active_states, pud_active_states, pud_query_holders, mhq_t, mhq_t, pud_query_holders>;
    using leaf_refuter_t =
        pud_leaf_refuter<pud_refuted_leaves, refutation_propagator_t, node_deactivator_t, pud_data, pud_refuted_leaves>;

    using acceptor_t = pud_acceptor<
        pud_node_goals, pud_data, pud_data, expr_pool, term_factory_t, term_factory_t, term_factory_t, term_factory_t>;
    using resolver_t = pud_resolver<pud_active_states, pud_node_goals, node_lvcs_t, mhq_t, mhcs_t, acceptor_t>;
    using expander_t = pud_expander<
        resolver_t, node_maker_t, pud_node_live_child_counts, child_activator_t, leaf_refuter_t, node_deactivator_t,
        pud_refuted_leaves>;

    pud_manifest();

    expr_pool                  expr_pool_;
    globalizer                 globalizer_;
    term_factory_t             term_factory_;
    pud_data                   data_;
    pud_refuted_leaves         refuted_leaves_;
    pud_node_goals             node_goals_;
    node_lvcs_t                node_lvcs_;
    node_axiom_groups_t        node_axiom_groups_;
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
    query_opener_t             query_opener_;
    query_forker_t             query_forker_;
    root_activator_t           root_activator_;
    child_activator_t          child_activator_;
    node_deactivator_t         node_deactivator_;
    leaf_refuter_t             leaf_refuter_;

    acceptor_t                 acceptor_;
    resolver_t                 resolver_;
    expander_t                 expander_;
};

#endif
