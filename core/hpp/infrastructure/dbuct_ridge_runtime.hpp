#ifndef DBUCT_RIDGE_RUNTIME_HPP
#define DBUCT_RIDGE_RUNTIME_HPP

#include <cstddef>
#include <cstdint>
#include "infrastructure/dbuct_ridge_manifest.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "value_objects/lemma.hpp"

// ridge_dbuct runtime — same session API as ridge_runtime, backed by the
// delayed-backtracking (camping) solver stack. grant_increment_interval controls
// DBUCT's per-node compute batch growth (larger ⇒ camps longer); it defaults so
// the constructor signature matches the other runtimes for shared harnesses.
struct dbuct_ridge_runtime {
    static constexpr size_t k_default_grant_increment_interval = 4;

    dbuct_ridge_runtime(
        db& database,
        initial_goal_exprs& goals,
        uint32_t initial_frame_offset,
        size_t max_resolutions,
        uint32_t random_seed,
        double exploration_constant,
        size_t grant_increment_interval = k_default_grant_increment_interval);

    bool next();
    bool solved() const;
    const expr* normalize(framed_expr fe);
    size_t resolution_depth() const;
    size_t decision_depth() const;
    lemma derive_decision_lemma() const;
    lemma derive_resolution_lemma() const;

private:
    dbuct_ridge_manifest manifest_;
};

#endif
