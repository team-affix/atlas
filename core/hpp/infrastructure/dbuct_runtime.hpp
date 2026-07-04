#ifndef DBUCT_RUNTIME_HPP
#define DBUCT_RUNTIME_HPP

#include <cstddef>
#include <cstdint>
#include "infrastructure/dbuct_manifest.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/normalizer.hpp"
#include "infrastructure/solver_driver.hpp"
#include "value_objects/lemma.hpp"

// ridge_dbuct runtime — same session API as ridge_runtime, backed by the
// delayed-backtracking (camping) solver stack. Accepts an optional
// grant_increment_interval controlling DBUCT's per-node compute batch growth
// (larger B ⇒ camps longer before backtracking); it defaults to a moderate value
// so the constructor signature matches the other runtimes for shared harnesses.
struct dbuct_runtime {
    static constexpr size_t kDefaultGrantIncrementInterval = 4;

    dbuct_runtime(
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
    using normalizer_t = normalizer<globalizer, expr_pool, expr_pool, dbuct_bind_map>;
    dbuct_manifest manifest_;
    normalizer_t normalizer_;
    solver_driver driver_;
};

inline dbuct_runtime::dbuct_runtime(
    db& database,
    initial_goal_exprs& goals,
    uint32_t initial_frame_offset,
    size_t max_resolutions,
    uint32_t random_seed,
    double exploration_constant,
    size_t grant_increment_interval)
    : manifest_(database, goals, initial_frame_offset, max_resolutions,
                random_seed, exploration_constant, grant_increment_interval),
      normalizer_(manifest_.globalizer_, manifest_.expr_pool_,
                  manifest_.expr_pool_, manifest_.bind_map_),
      driver_(manifest_.solver_.solve()) {}

inline bool dbuct_runtime::next() { return driver_.next(); }
inline bool dbuct_runtime::solved() const { return driver_.solved(); }
inline const expr* dbuct_runtime::normalize(framed_expr fe) { return normalizer_.normalize(fe); }
inline size_t dbuct_runtime::resolution_depth() const { return manifest_.resolution_memory_.get_resolution_count(); }
inline size_t dbuct_runtime::decision_depth() const { return manifest_.decision_memory_.count(); }
inline lemma dbuct_runtime::derive_decision_lemma() const { return manifest_.decision_memory_.derive_decision_lemma(); }
inline lemma dbuct_runtime::derive_resolution_lemma() const { return manifest_.resolution_memory_.derive_resolution_lemma(); }

#endif
