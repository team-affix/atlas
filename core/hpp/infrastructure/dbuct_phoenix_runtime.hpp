#ifndef DBUCT_PHOENIX_RUNTIME_HPP
#define DBUCT_PHOENIX_RUNTIME_HPP

#include <cstddef>
#include <cstdint>
#include "infrastructure/dbuct_phoenix_manifest.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "value_objects/lemma.hpp"

// phoenix_dbuct runtime — same session API as phoenix_runtime, backed by the
// delayed-backtracking (camping) solver stack. grant_k is the
// visit-proportional grant coefficient (grant(n) = 1 + k * visits(n); larger
// camps longer). It defaults so the constructor signature matches the other
// runtimes for shared harnesses.
struct dbuct_phoenix_runtime {
    static constexpr double k_default_grant_k = 0.1;

    dbuct_phoenix_runtime(
        db& database,
        initial_goal_exprs& goals,
        uint32_t initial_frame_offset,
        size_t max_resolutions,
        uint32_t random_seed,
        double exploration_constant,
        double grant_k = k_default_grant_k);

    bool next();
    bool solved() const;
    const expr* normalize(framed_expr fe);
    size_t resolution_depth() const;
    size_t decision_depth() const;
    lemma derive_decision_lemma() const;
    lemma derive_resolution_lemma() const;

private:
    dbuct_phoenix_manifest manifest_;
};

#endif
