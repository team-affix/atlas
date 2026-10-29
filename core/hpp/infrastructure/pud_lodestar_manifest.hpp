#ifndef PUD_LODESTAR_MANIFEST_HPP
#define PUD_LODESTAR_MANIFEST_HPP

#include <deque>
#include <optional>
#include <set>
#include <unordered_set>
#include <variant>
#include <vector>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_acceptor.hpp"
#include "infrastructure/pud_active_states.hpp"
#include "infrastructure/pud_binder.hpp"
#include "infrastructure/pud_bind_map_factory.hpp"
#include "infrastructure/pud_child_activator.hpp"
#include "infrastructure/pud_cycle_detector_factory.hpp"
#include "infrastructure/pud_exporter_factory.hpp"
#include "infrastructure/pud_goal_resolver.hpp"
#include "infrastructure/pud_initial_goal_opener.hpp"
#include "infrastructure/pud_leaf_refuter.hpp"
#include "infrastructure/pud_lodestar_candidate_watch.hpp"
#include "infrastructure/pud_lodestar_driver.hpp"
#include "infrastructure/pud_lodestar_frontier.hpp"
#include "infrastructure/ubs_bind_map.hpp"
#include "infrastructure/pud_lodestar_witness_log.hpp"
#include "infrastructure/pud_mhcs.hpp"
#include "infrastructure/pud_mhq.hpp"
#include "infrastructure/pud_mhws.hpp"
#include "infrastructure/pud_module_activator.hpp"
#include "infrastructure/pud_modules.hpp"
#include "infrastructure/pud_node_axioms.hpp"
#include "infrastructure/pud_node_deactivator.hpp"
#include "infrastructure/pud_node_goals.hpp"
#include "infrastructure/pud_node_live_child_counts.hpp"
#include "infrastructure/pud_query_holders.hpp"
#include "infrastructure/pud_refutation_propagator.hpp"
#include "infrastructure/pud_replayer.hpp"
#include "infrastructure/pud_resolver.hpp"
#include "infrastructure/pud_root_activator.hpp"
#include "infrastructure/pud_structure_watcher.hpp"
#include "infrastructure/pud_unifier_factory.hpp"
#include "infrastructure/pud_unfolder.hpp"
#include "value_objects/pud_global_node_id_hash.hpp"
#include "value_objects/pud_mhq_head_id.hpp"

struct pud_lodestar_manifest {
    struct bind_map_factory_t : pud_bind_map_factory {
        using pud_bind_map_factory::pud_bind_map_factory;
    };
    struct cycle_detector_factory_t : pud_cycle_detector_factory<globalizer, expr_pool> {
        using pud_cycle_detector_factory<globalizer, expr_pool>::pud_cycle_detector_factory;
    };
    struct unifier_factory_t : pud_unifier_factory<globalizer, expr_pool> {
        using pud_unifier_factory<globalizer, expr_pool>::pud_unifier_factory;
    };
    struct exporter_factory_t : pud_exporter_factory<globalizer, expr_pool, expr_pool> {
        using pud_exporter_factory<globalizer, expr_pool, expr_pool>::pud_exporter_factory;
    };
    struct modules_t : pud_modules<pud_module> {
        using pud_modules<pud_module>::pud_modules;
    };
    struct unit_queries_t : std::deque<pud_mhq_head_id> {
        using std::deque<pud_mhq_head_id>::deque;
    };
    struct refutation_propagator_t : pud_refutation_propagator<modules_t, pud_node_live_child_counts, modules_t> {
        using pud_refutation_propagator<modules_t, pud_node_live_child_counts, modules_t>::pud_refutation_propagator;
    };
    struct replayer_t : pud_replayer<
        modules_t, modules_t, expr_pool, bind_map_factory_t, cycle_detector_factory_t, unifier_factory_t> {
        using pud_replayer<
            modules_t, modules_t, expr_pool, bind_map_factory_t, cycle_detector_factory_t, unifier_factory_t>::pud_replayer;
    };
    struct mhws_t : pud_mhws<replayer_t, modules_t, modules_t, modules_t> {
        using pud_mhws<replayer_t, modules_t, modules_t, modules_t>::pud_mhws;
    };
    struct witness_log_t : pud_lodestar_witness_log<mhws_t, modules_t> {
        using pud_lodestar_witness_log<mhws_t, modules_t>::pud_lodestar_witness_log;
    };
    struct mhcs_t : pud_mhcs<
        replayer_t, witness_log_t, witness_log_t, witness_log_t, witness_log_t, witness_log_t, modules_t, modules_t,
        modules_t> {
        using pud_mhcs<
            replayer_t, witness_log_t, witness_log_t, witness_log_t, witness_log_t, witness_log_t, modules_t,
            modules_t, modules_t>::pud_mhcs;
    };
    struct candidate_watch_t : pud_lodestar_candidate_watch<mhcs_t, witness_log_t> {
        using pud_lodestar_candidate_watch<mhcs_t, witness_log_t>::pud_lodestar_candidate_watch;
    };
    struct mhq_t : pud_mhq<replayer_t, modules_t, candidate_watch_t, candidate_watch_t, candidate_watch_t, candidate_watch_t, unit_queries_t> {
        using pud_mhq<
            replayer_t, modules_t, candidate_watch_t, candidate_watch_t, candidate_watch_t, candidate_watch_t,
            unit_queries_t>::pud_mhq;
    };
    struct binder_t : pud_binder<bind_map_factory_t, modules_t> {
        using pud_binder<bind_map_factory_t, modules_t>::pud_binder;
    };
    struct structure_watcher_t : pud_structure_watcher<bind_map_factory_t, modules_t, expr_pool> {
        using pud_structure_watcher<bind_map_factory_t, modules_t, expr_pool>::pud_structure_watcher;
    };
    struct root_activator_t : pud_root_activator<
        modules_t, pud_initial_goal_opener, pud_node_goals, pud_node_axioms, pud_active_states, structure_watcher_t,
        mhq_t, pud_query_holders> {
        using pud_root_activator<
            modules_t, pud_initial_goal_opener, pud_node_goals, pud_node_axioms, pud_active_states, structure_watcher_t,
            mhq_t, pud_query_holders>::pud_root_activator;
    };
    struct child_activator_t : pud_child_activator<
        pud_active_states, pud_active_states, modules_t, pud_node_goals, pud_node_axioms, pud_goal_resolver,
        pud_node_goals, pud_node_goals, pud_node_axioms, pud_node_live_child_counts, binder_t, structure_watcher_t,
        mhq_t, mhq_t, pud_query_holders, mhq_t> {
        using pud_child_activator<
            pud_active_states, pud_active_states, modules_t, pud_node_goals, pud_node_axioms, pud_goal_resolver,
            pud_node_goals, pud_node_goals, pud_node_axioms, pud_node_live_child_counts, binder_t, structure_watcher_t,
            mhq_t, mhq_t, pud_query_holders, mhq_t>::pud_child_activator;
    };
    struct node_deactivator_t : pud_node_deactivator<
        pud_active_states, pud_active_states, pud_query_holders, mhq_t, mhq_t, pud_query_holders> {
        using pud_node_deactivator<
            pud_active_states, pud_active_states, pud_query_holders, mhq_t, mhq_t, pud_query_holders>::pud_node_deactivator;
    };
    struct leaf_refuter_t : pud_leaf_refuter<modules_t, refutation_propagator_t, node_deactivator_t, modules_t, modules_t> {
        using pud_leaf_refuter<modules_t, refutation_propagator_t, node_deactivator_t, modules_t, modules_t>::pud_leaf_refuter;
    };
    struct acceptor_t : pud_acceptor<
        pud_node_goals, modules_t, modules_t, expr_pool, expr_pool, bind_map_factory_t, cycle_detector_factory_t,
        unifier_factory_t, exporter_factory_t> {
        using pud_acceptor<
            pud_node_goals, modules_t, modules_t, expr_pool, expr_pool, bind_map_factory_t, cycle_detector_factory_t,
            unifier_factory_t, exporter_factory_t>::pud_acceptor;
    };
    struct resolver_t : pud_resolver<pud_active_states, pud_node_goals, mhq_t, mhcs_t, acceptor_t> {
        using pud_resolver<pud_active_states, pud_node_goals, mhq_t, mhcs_t, acceptor_t>::pud_resolver;
    };
    struct unfolder_t : pud_unfolder<resolver_t, modules_t, child_activator_t, leaf_refuter_t, node_deactivator_t, modules_t> {
        using pud_unfolder<resolver_t, modules_t, child_activator_t, leaf_refuter_t, node_deactivator_t, modules_t>::pud_unfolder;
    };
    struct module_activator_t : pud_module_activator<
        modules_t, modules_t, modules_t, root_activator_t, child_activator_t, leaf_refuter_t, node_deactivator_t> {
        using pud_module_activator<
            modules_t, modules_t, modules_t, root_activator_t, child_activator_t, leaf_refuter_t,
            node_deactivator_t>::pud_module_activator;
    };

    pud_lodestar_manifest(
        expr_pool& expr_pool,
        pud_global_node_id focus_axiom,
        size_t max_unfolds,
        size_t non_conjecture_unfolds_per_conjecture_unfold);
    void register_module(pud_module& module);
    bool on_focus(pud_global_node_id node) const;
    void note_offspring(pud_global_node_id parent, const std::vector<pud_global_node_id>& offspring);
    bool next();
    const expr* solution() const;
    size_t solution_depth() const;

    globalizer                 globalizer_;
    cycle_detector_factory_t   cycle_detector_factory_;
    unifier_factory_t          unifier_factory_;
    exporter_factory_t         exporter_factory_;
    bind_map_factory_t         bind_map_factory_;
    modules_t                  modules_;
    pud_node_goals             node_goals_;
    pud_node_axioms            node_axioms_;
    pud_goal_resolver          goal_resolver_;
    pud_initial_goal_opener    initial_goal_opener_;
    pud_node_live_child_counts live_child_counts_;
    refutation_propagator_t    refutation_propagator_;

    unit_queries_t             unit_queries_;
    replayer_t                 replayer_;
    mhws_t                     mhws_;
    witness_log_t              witness_log_;
    mhcs_t                     mhcs_;
    candidate_watch_t          candidate_watch_;
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

    expr_pool& exprs_;
    pud_global_node_id focus_axiom_;
    std::unordered_set<pud_global_node_id, pud_global_node_id_hash> focus_nodes_;
    size_t max_unfolds_;
    size_t unfolds_;
    size_t entry_unfolds_;
    size_t pending_conflicts_;
    pud_lodestar_frontier frontier_;
    pud_lodestar_driver<pud_lodestar_manifest> driver_;
    std::set<pud_global_node_id> reported_;
    const expr* solution_;
    size_t solution_depth_;
    bool stopped_;
private:
    std::optional<pud_global_node_id> unreported_answer() const;
    void seed_focus_tree();
    const expr* normalized_head(pud_global_node_id leaf) const;
    size_t depth_of(pud_global_node_id leaf) const;
    const expr* normalize_with_bindings(const ubs_bind_map& bind_map, const expr* skeleton) const;
};

#endif
