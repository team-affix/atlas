#ifndef PUD_MANIFEST_HPP
#define PUD_MANIFEST_HPP

#include <deque>
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_acceptor.hpp"
#include "infrastructure/pud_active_states.hpp"
#include "infrastructure/pud_binder.hpp"
#include "infrastructure/pud_child_activator.hpp"
#include "infrastructure/pud_leaf_refuter.hpp"
#include "infrastructure/pud_mhcs.hpp"
#include "infrastructure/pud_mhq.hpp"
#include "infrastructure/pud_mhws.hpp"
#include "infrastructure/pud_module_activator.hpp"
#include "infrastructure/pud_modules.hpp"
#include "infrastructure/pud_node_deactivator.hpp"
#include "infrastructure/pud_node_goals.hpp"
#include "infrastructure/pud_node_live_child_counts.hpp"
#include "infrastructure/pud_query_holders.hpp"
#include "infrastructure/pud_refutation_propagator.hpp"
#include "infrastructure/pud_replayer.hpp"
#include "infrastructure/pud_resolver.hpp"
#include "infrastructure/pud_root_activator.hpp"
#include "infrastructure/pud_structure_watcher.hpp"
#include "infrastructure/pud_term_factory.hpp"
#include "infrastructure/pud_unfolder.hpp"
#include "value_objects/pud_mhq_head_id.hpp"

// Composition root of the runtime: every infrastructure that works on the registered modules.
// The modules and the expression pool are the caller's; every expression the runtime makes goes
// into that pool.
template<typename IModule, typename IExprPool>
struct pud_manifest {
    using term_factory_t = pud_term_factory<globalizer, IExprPool, IExprPool>;
    using modules_t = pud_modules<IModule>;
    using unit_queries_t = std::deque<pud_mhq_head_id>;

    using refutation_propagator_t = pud_refutation_propagator<modules_t, pud_node_live_child_counts, modules_t>;

    using replayer_t = pud_replayer<modules_t, modules_t, IExprPool, term_factory_t, term_factory_t, term_factory_t>;
    using mhws_t = pud_mhws<replayer_t, modules_t, modules_t, modules_t>;
    using mhcs_t = pud_mhcs<replayer_t, mhws_t, mhws_t, mhws_t, mhws_t, mhws_t, modules_t, modules_t, modules_t>;
    using mhq_t = pud_mhq<replayer_t, modules_t, mhcs_t, mhcs_t, mhcs_t, mhcs_t, unit_queries_t>;

    using binder_t = pud_binder<term_factory_t, modules_t>;
    using structure_watcher_t = pud_structure_watcher<term_factory_t, modules_t>;
    using root_activator_t =
        pud_root_activator<modules_t, pud_node_goals, pud_active_states, structure_watcher_t, mhq_t, pud_query_holders>;
    using child_activator_t = pud_child_activator<
        pud_active_states, pud_active_states, modules_t, pud_node_goals, pud_node_goals, pud_node_goals,
        pud_node_live_child_counts, binder_t, structure_watcher_t, mhq_t, mhq_t, pud_query_holders, mhq_t>;
    using node_deactivator_t = pud_node_deactivator<
        pud_active_states, pud_active_states, pud_query_holders, mhq_t, mhq_t, pud_query_holders>;
    using leaf_refuter_t =
        pud_leaf_refuter<modules_t, refutation_propagator_t, node_deactivator_t, modules_t, modules_t>;

    using acceptor_t = pud_acceptor<
        pud_node_goals, modules_t, modules_t, IExprPool, term_factory_t, term_factory_t, term_factory_t,
        term_factory_t>;
    using resolver_t = pud_resolver<pud_active_states, pud_node_goals, mhq_t, mhcs_t, acceptor_t>;
    using unfolder_t =
        pud_unfolder<resolver_t, modules_t, child_activator_t, leaf_refuter_t, node_deactivator_t, modules_t>;
    using module_activator_t = pud_module_activator<
        modules_t, modules_t, modules_t, root_activator_t, child_activator_t, leaf_refuter_t, node_deactivator_t>;

    pud_manifest(IExprPool& expr_pool);

    globalizer                 globalizer_;
    term_factory_t             term_factory_;
    modules_t                  modules_;
    pud_node_goals             node_goals_;
    pud_node_live_child_counts live_child_counts_;
    refutation_propagator_t    refutation_propagator_;

    unit_queries_t             unit_queries_;
    replayer_t                 replayer_;
    mhws_t                     mhws_;
    mhcs_t                     mhcs_;
    mhq_t                      mhq_;

    pud_active_states          active_states_;
    pud_query_holders          query_holders_;
    binder_t                   binder_;
    structure_watcher_t        structure_watcher_;
    root_activator_t           root_activator_;
    child_activator_t          child_activator_;
    node_deactivator_t         node_deactivator_;
    leaf_refuter_t             leaf_refuter_;

    acceptor_t                 acceptor_;
    resolver_t                 resolver_;
    unfolder_t                 unfolder_;
    module_activator_t         module_activator_;
};

template<typename IModule, typename IExprPool>
pud_manifest<IModule, IExprPool>::pud_manifest(IExprPool& expr_pool)
    : globalizer_()
    , term_factory_(globalizer_, expr_pool, expr_pool)
    , modules_()
    , node_goals_()
    , live_child_counts_()
    , refutation_propagator_(modules_, live_child_counts_, modules_)
    , unit_queries_()
    , replayer_(modules_, modules_, expr_pool, term_factory_, term_factory_, term_factory_)
    , mhws_(replayer_, modules_, modules_, modules_)
    , mhcs_(replayer_, mhws_, mhws_, mhws_, mhws_, mhws_, modules_, modules_, modules_)
    , mhq_(replayer_, modules_, mhcs_, mhcs_, mhcs_, mhcs_, unit_queries_)
    , active_states_()
    , query_holders_()
    , binder_(term_factory_, modules_)
    , structure_watcher_(term_factory_, modules_)
    , root_activator_(modules_, node_goals_, active_states_, structure_watcher_, mhq_, query_holders_)
    , child_activator_(
          active_states_, active_states_, modules_, node_goals_, node_goals_, node_goals_, live_child_counts_, binder_,
          structure_watcher_, mhq_, mhq_, query_holders_, mhq_)
    , node_deactivator_(active_states_, active_states_, query_holders_, mhq_, mhq_, query_holders_)
    , leaf_refuter_(modules_, refutation_propagator_, node_deactivator_, modules_, modules_)
    , acceptor_(node_goals_, modules_, modules_, expr_pool, term_factory_, term_factory_, term_factory_, term_factory_)
    , resolver_(active_states_, node_goals_, mhq_, mhcs_, acceptor_)
    , unfolder_(resolver_, modules_, child_activator_, leaf_refuter_, node_deactivator_, modules_)
    , module_activator_(
          modules_, modules_, modules_, root_activator_, child_activator_, leaf_refuter_, node_deactivator_) {}

#endif
