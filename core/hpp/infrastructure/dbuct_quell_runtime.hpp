#ifndef DBUCT_QUELL_RUNTIME_HPP
#define DBUCT_QUELL_RUNTIME_HPP

#include <cstddef>
#include <cstdint>
#include "infrastructure/dbuct_quell_manifest.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "value_objects/lemma.hpp"

// dbuct-quell runtime — camping DBUCT search with quell (remaining-work) reward.
struct dbuct_quell_runtime {
    static constexpr size_t k_default_grant_increment_interval = 4;

    dbuct_quell_runtime(
        db& database,
        initial_goal_exprs& goals,
        uint32_t initial_frame_offset,
        size_t max_resolutions,
        uint32_t random_seed,
        double exploration_constant,
        double work_decay_k,
        double work_decay_j,
        size_t grant_increment_interval = k_default_grant_increment_interval);

    bool next();
    bool solved() const;
    const expr* normalize(framed_expr fe);
    size_t resolution_depth() const;
    size_t decision_depth() const;
    double remaining_work() const;
    lemma derive_decision_lemma() const;
    lemma derive_resolution_lemma() const;

private:
    dbuct_quell_manifest manifest_;
};

#endif
