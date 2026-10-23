#include "infrastructure/pud_manifest.hpp"

pud_manifest::pud_manifest()
    : expr_pool_()
    , globalizer_()
    , term_factory_(globalizer_, expr_pool_, expr_pool_)
    , data_()
    , refuted_leaves_()
    , node_goals_()
    , node_lvcs_()
    , node_axiom_groups_()
    , live_child_counts_()
    , refuted_node_checker_(data_, refuted_leaves_, live_child_counts_)
    , axiom_groups_()
    , database_axiom_group_(axiom_groups_.create_group())
    , refutation_propagator_(data_, live_child_counts_)
    , node_maker_(
          data_, data_, node_goals_, node_goals_, node_lvcs_, node_lvcs_, node_axiom_groups_, node_axiom_groups_,
          axiom_groups_)
    , unit_queries_()
    , replayer_(data_, data_, expr_pool_, term_factory_, term_factory_, term_factory_)
    , mhws_(replayer_, data_, refuted_node_checker_, data_)
    , mhcs_(replayer_, mhws_, mhws_, mhws_, mhws_, mhws_, data_, refuted_node_checker_, data_)
    , mhq_(replayer_, axiom_groups_, mhcs_, mhcs_, mhcs_, mhcs_, unit_queries_)
    , active_states_()
    , query_holders_()
    , binder_(term_factory_, data_)
    , structure_watcher_(term_factory_, data_)
    , query_opener_(mhq_, node_goals_, node_lvcs_, node_axiom_groups_)
    , query_forker_(mhq_, node_goals_, node_lvcs_)
    , root_activator_(active_states_, structure_watcher_, node_goals_, query_opener_, query_holders_)
    , child_activator_(
          active_states_, active_states_, binder_, structure_watcher_, node_goals_, data_, query_forker_,
          query_opener_, query_holders_, mhq_)
    , node_deactivator_(active_states_, active_states_, query_holders_, mhq_, mhq_, query_holders_)
    , leaf_refuter_(refuted_leaves_, refutation_propagator_, node_deactivator_, data_, refuted_leaves_)
    , acceptor_(
          node_goals_, data_, data_, expr_pool_, term_factory_, term_factory_, term_factory_, term_factory_)
    , resolver_(active_states_, node_goals_, node_lvcs_, mhq_, mhcs_, acceptor_)
    , expander_(
          resolver_, node_maker_, live_child_counts_, child_activator_, leaf_refuter_, node_deactivator_,
          refuted_leaves_) {}
