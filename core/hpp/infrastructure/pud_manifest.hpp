#ifndef PUD_MANIFEST_HPP
#define PUD_MANIFEST_HPP

#include "infrastructure/expr_pool.hpp"
#include "infrastructure/fully_persistent_array.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/order_maintenance.hpp"
#include "infrastructure/pud_axiom_adder.hpp"
#include "infrastructure/pud_candidate_search.hpp"
#include "infrastructure/pud_node_added_body_goals.hpp"
#include "infrastructure/pud_node_added_touched_caller_reps.hpp"
#include "infrastructure/pud_node_added_unifications.hpp"
#include "infrastructure/pud_node_children.hpp"
#include "infrastructure/pud_node_interval.hpp"
#include "infrastructure/pud_node_lvc.hpp"
#include "infrastructure/pud_node_parent.hpp"
#include "infrastructure/pud_normalizer.hpp"
#include "infrastructure/pud_queries.hpp"
#include "infrastructure/pud_rule_id_pool.hpp"
#include "infrastructure/pud_unfolder.hpp"
#include "infrastructure/pud_witness_search.hpp"

struct pud_manifest {
    using normalizer_t = pud_normalizer<
        globalizer, fully_persistent_array, fully_persistent_array,
        expr_pool, expr_pool>;
    using witness_search_t = pud_witness_search<
        pud_node_children, pud_node_parent, pud_rule_id_pool,
        pud_node_interval, pud_node_interval, pud_node_interval,
        order_maintenance, pud_node_added_unifications,
        fully_persistent_array, fully_persistent_array,
        globalizer, expr_pool, pud_node_added_touched_caller_reps>;
    using candidate_search_t = pud_candidate_search<
        witness_search_t, pud_node_children, pud_node_parent,
        pud_node_added_body_goals>;
    using queries_t = pud_queries<
        pud_node_added_body_goals, pud_node_lvc,
        candidate_search_t, witness_search_t>;
    using unfolder_t = pud_unfolder<
        queries_t, normalizer_t, normalizer_t, normalizer_t, expr_pool,
        pud_node_lvc, pud_rule_id_pool,
        pud_node_added_unifications, pud_node_added_unifications, pud_node_added_body_goals, pud_node_lvc,
        pud_node_children, pud_node_parent, pud_node_parent, pud_node_interval,
        order_maintenance, pud_node_interval, fully_persistent_array,
        pud_node_added_touched_caller_reps, queries_t>;
    using axiom_adder_t = pud_axiom_adder<
        pud_rule_id_pool, pud_node_added_unifications, pud_node_added_body_goals,
        pud_node_lvc, pud_node_parent, order_maintenance,
        pud_node_interval, queries_t>;

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
    pud_node_interval node_interval_;
    pud_node_added_touched_caller_reps added_caller_reps_;
    normalizer_t pud_normalizer_;
    witness_search_t witness_search_;
    candidate_search_t candidate_search_;
    queries_t queries_;
    unfolder_t unfolder_;
    axiom_adder_t axiom_adder_;
};

#endif
