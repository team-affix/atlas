#ifndef BASIC_RUNTIME_HPP
#define BASIC_RUNTIME_HPP

#include <cstdint>
#include "infrastructure/basic_manifest.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/normalizer.hpp"
#include "infrastructure/solver_driver.hpp"
#include "value_objects/lemma.hpp"

struct basic_runtime {
    basic_runtime(
        db& database,
        initial_goal_exprs& goals,
        uint32_t initial_frame_offset,
        size_t max_resolutions,
        uint32_t random_seed);

    bool next();
    bool solved() const;
    size_t resolution_depth() const;
    size_t decision_depth() const;
    const expr* normalize(framed_expr);
    lemma derive_decision_lemma() const;
    lemma derive_resolution_lemma() const;

private:
    using normalizer_t = normalizer<globalizer, expr_pool, expr_pool, basic_manifest::bind_map_t>;

    basic_manifest manifest_;
    normalizer_t normalizer_;
    solver_driver driver_;
};

#endif
