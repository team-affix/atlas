#include "infrastructure/rule_unfolder_runtime.hpp"

rule_unfolder_runtime::rule_unfolder_runtime(db& original_db)
    : expanded_db_(original_db)
    , manifest_(original_db, expanded_db_) {}

std::vector<rule_id> rule_unfolder_runtime::unfold(
    rule_id subject_id, subgoal_id body_idx) {
    return manifest_.unfold(subject_id, body_idx);
}
