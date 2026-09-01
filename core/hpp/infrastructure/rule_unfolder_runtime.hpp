#ifndef RULE_UNFOLDER_RUNTIME_HPP
#define RULE_UNFOLDER_RUNTIME_HPP

#include <vector>
#include "infrastructure/db.hpp"
#include "infrastructure/rule_unfolder_manifest.hpp"
#include "value_objects/lineage.hpp"

struct rule_unfolder_runtime {
    rule_unfolder_runtime(db& original_db);

    std::vector<rule_id> unfold(rule_id subject_id, subgoal_id body_idx);

private:
    db                     expanded_db_;
    rule_unfolder_manifest manifest_;
};

#endif
