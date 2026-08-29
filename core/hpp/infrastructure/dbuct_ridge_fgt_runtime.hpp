#ifndef DBUCT_RIDGE_FGT_RUNTIME_HPP
#define DBUCT_RIDGE_FGT_RUNTIME_HPP

#include <cstddef>
#include <cstdint>
#include "infrastructure/dbuct_ridge_fgt_manifest.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "value_objects/lemma.hpp"

struct dbuct_ridge_fgt_runtime {
    static constexpr double k_default_grant_k    = 0.1;
    static constexpr size_t k_default_max_clauses = 10000;

    dbuct_ridge_fgt_runtime(
        db& database,
        initial_goal_exprs& goals,
        uint32_t initial_frame_offset,
        size_t max_resolutions,
        uint32_t random_seed,
        double exploration_constant,
        double grant_k = k_default_grant_k,
        size_t max_clauses = k_default_max_clauses);

    bool next();
    bool solved() const;
    const expr* normalize(framed_expr fe);
    size_t resolution_depth() const;
    size_t decision_depth() const;
    lemma derive_decision_lemma() const;
    lemma derive_resolution_lemma() const;

private:
    dbuct_ridge_fgt_manifest manifest_;
};

#endif
