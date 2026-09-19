#ifndef PUD_MANIFEST_HPP
#define PUD_MANIFEST_HPP

#include "infrastructure/expr_pool.hpp"
#include "infrastructure/fully_persistent_array.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/order_maintenance.hpp"
#include "infrastructure/pud.hpp"
#include "infrastructure/pud_candidate_search.hpp"
#include "infrastructure/pud_invalidation_router.hpp"
#include "infrastructure/pud_leaf_queries.hpp"
#include "infrastructure/pud_query_reinitializer.hpp"
#include "infrastructure/pud_rule_id_pool.hpp"
#include "infrastructure/pud_unfolder.hpp"
#include "infrastructure/pud_unify_head.hpp"
#include "infrastructure/pud_witness_search.hpp"
#include "infrastructure/pud_witness_watchers.hpp"

struct pud_manifest {
    using forest_t = pud<pud_rule_id_pool, pud_rule_id_pool,
                         order_maintenance, order_maintenance>;
    using unify_head_t = pud_unify_head<
        order_maintenance, forest_t, forest_t,
        fully_persistent_array, fully_persistent_array,
        globalizer, expr_pool, expr_pool>;
    using witness_search_t = pud_witness_search<
        forest_t, forest_t, forest_t, unify_head_t>;
    using candidate_search_t = pud_candidate_search<
        witness_search_t, forest_t, forest_t, forest_t, unify_head_t>;
    using reinit_t = pud_query_reinitializer<
        forest_t, forest_t, fully_persistent_array, unify_head_t, unify_head_t>;
    using router_t = pud_invalidation_router<
        pud_witness_watchers, witness_search_t, candidate_search_t, unify_head_t>;
    using unfolder_t = pud_unfolder<
        forest_t, unify_head_t, unify_head_t, unify_head_t, expr_pool,
        forest_t, forest_t, order_maintenance,
        pud_leaf_queries, pud_leaf_queries, pud_leaf_queries,
        reinit_t, candidate_search_t,
        forest_t, forest_t, forest_t,
        router_t, pud_witness_watchers, pud_witness_watchers>;

    pud_manifest();

    pud_rule_id_pool pool_;
    order_maintenance om_;
    fully_persistent_array fpa_;
    globalizer globalizer_;
    expr_pool exprs_;
    forest_t forest_;
    pud_leaf_queries leaf_queries_;
    pud_witness_watchers watchers_;
    unify_head_t unify_head_;
    witness_search_t witness_search_;
    candidate_search_t candidate_search_;
    reinit_t reinit_;
    router_t router_;
    unfolder_t unfolder_;
};

#endif
