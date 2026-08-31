#ifndef RULE_UNFOLDER_MANIFEST_HPP
#define RULE_UNFOLDER_MANIFEST_HPP

#include <vector>
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/rule_unfolder.hpp"
#include "value_objects/lineage.hpp"

struct rule_unfolder_manifest {
    using rule_unfolder_t = rule_unfolder<db, db, db, db, db, db, expr_pool, expr_pool>;

    rule_unfolder_manifest(db& original_db, db& expanded_db);

    std::vector<rule_id> unfold(rule_id subject_id, subgoal_id body_idx);

    expr_pool        expr_pool_;
    rule_unfolder_t  rule_unfolder_;
};

#endif
