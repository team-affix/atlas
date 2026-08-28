#ifndef RIDGE_FGT_RUNTIME_HPP
#define RIDGE_FGT_RUNTIME_HPP

#include <cstddef>
#include <cstdint>
#include "infrastructure/ridge_fgt_manifest.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "value_objects/lemma.hpp"

struct ridge_fgt_runtime {
    ridge_fgt_runtime(
        db& database,
        initial_goal_exprs& goals,
        uint32_t initial_frame_offset,
        size_t max_resolutions,
        uint32_t random_seed,
        double exploration_constant,
        size_t max_clauses);

    bool next();
    bool solved() const;
    size_t resolution_depth() const;
    size_t decision_depth() const;
    const expr* normalize(framed_expr);
    lemma derive_decision_lemma() const;
    lemma derive_resolution_lemma() const;

private:
    ridge_fgt_manifest manifest_;
};

#endif
