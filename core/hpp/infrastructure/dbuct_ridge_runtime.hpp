#ifndef DBUCT_RIDGE_RUNTIME_HPP
#define DBUCT_RIDGE_RUNTIME_HPP

#include <cstddef>
#include <cstdint>
#include "infrastructure/dbuct_ridge_manifest.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/normalizer.hpp"
#include "infrastructure/solver_driver.hpp"
#include "value_objects/lemma.hpp"

// ridge_dbuct runtime — same session API as ridge_runtime, backed by the
// delayed-backtracking (camping) solver stack. grant_increment_interval controls
// DBUCT's per-node compute batch growth (larger ⇒ camps longer); it defaults so
// the constructor signature matches the other runtimes for shared harnesses.
struct dbuct_ridge_runtime {
    static constexpr size_t kDefaultGrantIncrementInterval = 4;

    dbuct_ridge_runtime(
        db& database,
        initial_goal_exprs& goals,
        uint32_t initial_frame_offset,
        size_t max_resolutions,
        uint32_t random_seed,
        double exploration_constant,
        size_t grant_increment_interval = kDefaultGrantIncrementInterval);

    bool next();
    bool solved() const;
    const expr* normalize(framed_expr fe);
    size_t resolution_depth() const;
    size_t decision_depth() const;
    lemma derive_decision_lemma() const;
    lemma derive_resolution_lemma() const;

private:
    using normalizer_t = normalizer<globalizer, expr_pool, expr_pool, dbuct_ridge_manifest::bind_map_t>;
    dbuct_ridge_manifest manifest_;
    normalizer_t normalizer_;
    solver_driver driver_;
};

#endif
