#ifndef QUELL_RUNTIME_HPP
#define QUELL_RUNTIME_HPP

#include <cstdint>
#include "infrastructure/quell_manifest.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "value_objects/lemma.hpp"

struct quell_runtime {
    quell_runtime(
        db& database,
        initial_goal_exprs& goals,
        uint32_t initial_frame_offset,
        size_t max_resolutions,
        uint32_t random_seed,
        double exploration_constant,
        double work_decay_k,
        double work_decay_j);

    bool next();
    bool solved() const;
    size_t resolution_depth() const;
    size_t decision_depth() const;
    double remaining_work() const;
    size_t remaining_active_goals() const;
    const expr* normalize(framed_expr);
    lemma derive_decision_lemma() const;
    lemma derive_resolution_lemma() const;

private:
    quell_manifest manifest_;
};

#endif
