#ifndef RULE_UNFOLDER_MANIFEST_HPP
#define RULE_UNFOLDER_MANIFEST_HPP

#include <vector>
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/expr_querier.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/rule_unfolder.hpp"
#include "value_objects/lineage.hpp"

struct rule_unfolder_manifest {
    using expr_querier_t  = expr_querier<db, db>;
    using rule_unfolder_t = rule_unfolder<db, db, db, expr_querier_t, db, expr_pool, expr_pool, globalizer>;

    rule_unfolder_manifest(db& original_db, db& expanded_db);

    std::vector<rule_id> unfold(rule_id subject_id, subgoal_id body_idx);

    expr_pool        expr_pool_;
    globalizer       globalizer_;
    expr_querier_t   expr_querier_;
    rule_unfolder_t  rule_unfolder_;
};

#endif
