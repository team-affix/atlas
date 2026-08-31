#include "infrastructure/rule_unfolder_manifest.hpp"

rule_unfolder_manifest::rule_unfolder_manifest(db& original_db, db& expanded_db)
    : expr_pool_()
    , rule_unfolder_(expanded_db, expanded_db, expanded_db,
                     original_db, original_db, original_db,
                     expr_pool_, expr_pool_) {}

std::vector<rule_id> rule_unfolder_manifest::unfold(rule_id subject_id, subgoal_id body_idx) {
    return rule_unfolder_.unfold(subject_id, body_idx);
}
