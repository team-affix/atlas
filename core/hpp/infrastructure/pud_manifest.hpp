#ifndef PUD_MANIFEST_HPP
#define PUD_MANIFEST_HPP

#include <cstdint>
#include <deque>
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_acceptor.hpp"
#include "infrastructure/pud_active_states.hpp"
#include "infrastructure/pud_binder.hpp"
#include "infrastructure/pud_child_activator.hpp"
#include "infrastructure/pud_expander.hpp"
#include "infrastructure/pud_leaf_refuter.hpp"
#include "infrastructure/pud_mhcs.hpp"
#include "infrastructure/pud_mhq.hpp"
#include "infrastructure/pud_mhws.hpp"
#include "infrastructure/pud_module_activator.hpp"
#include "infrastructure/pud_modules.hpp"
#include "infrastructure/pud_node_deactivator.hpp"
#include "infrastructure/pud_node_goals.hpp"
#include "infrastructure/pud_node_indexer.hpp"
#include "infrastructure/pud_node_live_child_counts.hpp"
#include "infrastructure/pud_node_table.hpp"
#include "infrastructure/pud_query_forker.hpp"
#include "infrastructure/pud_query_holders.hpp"
#include "infrastructure/pud_query_opener.hpp"
#include "infrastructure/pud_refutation_propagator.hpp"
#include "infrastructure/pud_replayer.hpp"
#include "infrastructure/pud_resolver.hpp"
#include "infrastructure/pud_root_activator.hpp"
#include "infrastructure/pud_structure_watcher.hpp"
#include "infrastructure/pud_term_factory.hpp"
#include "value_objects/pud_mhq_head_id.hpp"

// Composition root of the controller: every infrastructure that works on the registered modules.
// The modules and the expression pool are the caller's; every expression the controller makes goes
// into that pool.
template<typename IModuleData, typename IExprPool>
struct pud_manifest {
    using term_factory_t = pud_term_factory<globalizer, IExprPool, IExprPool>;
    using modules_t = pud_modules<IModuleData>;
    using node_lvcs_t = pud_node_table<uint32_t>;
    using unit_queries_t = std::deque<pud_mhq_head_id>;

    using refutation_propagator_t = pud_refutation_propagator<modules_t, pud_node_live_child_counts, modules_t>;
    using node_indexer_t =
        pud_node_indexer<modules_t, modules_t, pud_node_goals, pud_node_goals, node_lvcs_t, node_lvcs_t>;

    using replayer_t = pud_replayer<modules_t, modules_t, IExprPool, term_factory_t, term_factory_t, term_factory_t>;
    using mhws_t = pud_mhws<replayer_t, modules_t, modules_t, modules_t>;
    using mhcs_t = pud_mhcs<replayer_t, mhws_t, mhws_t, mhws_t, mhws_t, mhws_t, modules_t, modules_t, modules_t>;
    using mhq_t = pud_mhq<replayer_t, modules_t, mhcs_t, mhcs_t, mhcs_t, mhcs_t, unit_queries_t>;

    using binder_t = pud_binder<term_factory_t, modules_t>;
    using structure_watcher_t = pud_structure_watcher<term_factory_t, modules_t>;
    using query_opener_t = pud_query_opener<mhq_t, pud_node_goals, node_lvcs_t>;
    using query_forker_t = pud_query_forker<mhq_t, pud_node_goals, node_lvcs_t>;
    using root_activator_t =
        pud_root_activator<pud_active_states, structure_watcher_t, pud_node_goals, query_opener_t, pud_query_holders>;
    using child_activator_t = pud_child_activator<
        pud_active_states, pud_active_states, binder_t, structure_watcher_t, pud_node_goals, modules_t,
        query_forker_t, query_opener_t, pud_query_holders, mhq_t>;
    using node_deactivator_t = pud_node_deactivator<
        pud_active_states, pud_active_states, pud_query_holders, mhq_t, mhq_t, pud_query_holders>;
    using leaf_refuter_t =
        pud_leaf_refuter<modules_t, refutation_propagator_t, node_deactivator_t, modules_t, modules_t>;

    using acceptor_t = pud_acceptor<
        pud_node_goals, modules_t, modules_t, IExprPool, term_factory_t, term_factory_t, term_factory_t,
        term_factory_t>;
    using resolver_t = pud_resolver<pud_active_states, pud_node_goals, node_lvcs_t, mhq_t, mhcs_t, acceptor_t>;
    using expander_t = pud_expander<
        resolver_t, modules_t, node_indexer_t, pud_node_live_child_counts, child_activator_t, leaf_refuter_t,
        node_deactivator_t, modules_t>;
    using module_activator_t = pud_module_activator<
        modules_t, modules_t, modules_t, node_indexer_t, node_indexer_t, root_activator_t,
        pud_node_live_child_counts, child_activator_t, leaf_refuter_t, node_deactivator_t>;

    pud_manifest(IExprPool& expr_pool);

    globalizer                 globalizer_;
    term_factory_t             term_factory_;
    modules_t                  modules_;
    pud_node_goals             node_goals_;
    node_lvcs_t                node_lvcs_;
    pud_node_live_child_counts live_child_counts_;
    refutation_propagator_t    refutation_propagator_;
    node_indexer_t             node_indexer_;

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
    module_activator_t         module_activator_;
};

template<typename IModuleData, typename IExprPool>
pud_manifest<IModuleData, IExprPool>::pud_manifest(IExprPool& expr_pool)
    : globalizer_()
    , term_factory_(globalizer_, expr_pool, expr_pool)
    , modules_()
    , node_goals_()
    , node_lvcs_()
    , live_child_counts_()
    , refutation_propagator_(modules_, live_child_counts_, modules_)
    , node_indexer_(modules_, modules_, node_goals_, node_goals_, node_lvcs_, node_lvcs_)
    , unit_queries_()
    , replayer_(modules_, modules_, expr_pool, term_factory_, term_factory_, term_factory_)
    , mhws_(replayer_, modules_, modules_, modules_)
    , mhcs_(replayer_, mhws_, mhws_, mhws_, mhws_, mhws_, modules_, modules_, modules_)
    , mhq_(replayer_, modules_, mhcs_, mhcs_, mhcs_, mhcs_, unit_queries_)
    , active_states_()
    , query_holders_()
    , binder_(term_factory_, modules_)
    , structure_watcher_(term_factory_, modules_)
    , query_opener_(mhq_, node_goals_, node_lvcs_)
    , query_forker_(mhq_, node_goals_, node_lvcs_)
    , root_activator_(active_states_, structure_watcher_, node_goals_, query_opener_, query_holders_)
    , child_activator_(
          active_states_, active_states_, binder_, structure_watcher_, node_goals_, modules_, query_forker_,
          query_opener_, query_holders_, mhq_)
    , node_deactivator_(active_states_, active_states_, query_holders_, mhq_, mhq_, query_holders_)
    , leaf_refuter_(modules_, refutation_propagator_, node_deactivator_, modules_, modules_)
    , acceptor_(node_goals_, modules_, modules_, expr_pool, term_factory_, term_factory_, term_factory_, term_factory_)
    , resolver_(active_states_, node_goals_, node_lvcs_, mhq_, mhcs_, acceptor_)
    , expander_(
          resolver_, modules_, node_indexer_, live_child_counts_, child_activator_, leaf_refuter_, node_deactivator_,
          modules_)
    , module_activator_(
          modules_, modules_, modules_, node_indexer_, node_indexer_, root_activator_, live_child_counts_,
          child_activator_, leaf_refuter_, node_deactivator_) {}

#endif
