#ifndef HORIZON_RUNTIME_HPP
#define HORIZON_RUNTIME_HPP

#include <cstdint>
#include "infrastructure/horizon_manifest.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/normalizer.hpp"
#include "infrastructure/solver_driver.hpp"
#include "value_objects/lemma.hpp"

struct horizon_runtime {
    horizon_runtime(
        db& database,
        initial_goal_exprs& goals,
        uint32_t initial_frame_offset,
        size_t max_resolutions,
        uint32_t random_seed,
        double exploration_constant);

    bool next();
    bool solved() const;
    size_t resolution_depth() const;
    size_t decision_depth() const;
    double cgw() const;
    const expr* normalize(framed_expr);
    lemma derive_decision_lemma() const;
    lemma derive_resolution_lemma() const;

private:
    using normalizer_t = normalizer<globalizer, expr_pool, expr_pool, horizon_manifest::bind_map_t>;
    horizon_manifest manifest_;
    normalizer_t normalizer_;
    solver_driver driver_;
};

#endif
