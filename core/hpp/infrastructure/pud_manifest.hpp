#ifndef PUD_MANIFEST_HPP
#define PUD_MANIFEST_HPP

#include "infrastructure/expr_pool.hpp"
#include "infrastructure/fully_persistent_array.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/order_maintenance.hpp"
#include "infrastructure/pud_axiom_adder.hpp"
#include "infrastructure/pud_candidate_search.hpp"
#include "infrastructure/pud_node_added_body_goals.hpp"
#include "infrastructure/pud_node_added_unifications.hpp"
#include "infrastructure/pud_node_base_interval.hpp"
#include "infrastructure/pud_node_children.hpp"
#include "infrastructure/pud_node_lvc.hpp"
#include "infrastructure/pud_node_parent.hpp"
#include "infrastructure/pud_queries.hpp"
#include "infrastructure/pud_rule_id_pool.hpp"
#include "infrastructure/pud_unfolder.hpp"
#include "infrastructure/pud_unify_head.hpp"
#include "infrastructure/pud_witness_search.hpp"

struct pud_manifest {
    using unify_head_t = pud_unify_head<
        order_maintenance, pud_node_parent, pud_node_added_unifications,
        fully_persistent_array, fully_persistent_array,
        globalizer, expr_pool, expr_pool>;
    using witness_search_t = pud_witness_search<
        pud_node_children, pud_node_parent, unify_head_t>;
    using candidate_search_t = pud_candidate_search<
        witness_search_t, pud_node_children, pud_node_parent,
        unify_head_t, pud_node_added_body_goals>;
    using queries_t = pud_queries<
        pud_node_added_body_goals, pud_node_lvc, pud_node_base_interval,
        order_maintenance, unify_head_t, candidate_search_t, witness_search_t>;
    using unfolder_t = pud_unfolder<
        queries_t, unify_head_t, unify_head_t, expr_pool,
        pud_node_lvc, pud_rule_id_pool,
        pud_node_added_unifications, pud_node_added_body_goals, pud_node_lvc,
        pud_node_children, pud_node_parent, pud_node_base_interval, order_maintenance,
        pud_node_base_interval, queries_t>;
    using axiom_adder_t = pud_axiom_adder<
        pud_rule_id_pool, pud_node_added_unifications, pud_node_added_body_goals,
        pud_node_lvc, pud_node_parent, order_maintenance,
        pud_node_base_interval, queries_t>;

    pud_manifest();

    pud_rule_id_pool pool_;
    order_maintenance om_;
    fully_persistent_array fpa_;
    globalizer globalizer_;
    expr_pool exprs_;
    pud_node_added_unifications added_unifications_;
    pud_node_added_body_goals added_body_goals_;
    pud_node_lvc lvc_;
    pud_node_children children_;
    pud_node_parent parent_;
    pud_node_base_interval base_interval_;
    unify_head_t unify_head_;
    witness_search_t witness_search_;
    candidate_search_t candidate_search_;
    queries_t queries_;
    unfolder_t unfolder_;
    axiom_adder_t axiom_adder_;
};

#endif
