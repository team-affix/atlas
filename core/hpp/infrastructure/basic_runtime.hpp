#ifndef BASIC_RUNTIME_HPP
#define BASIC_RUNTIME_HPP

#include <cstdint>
#include "infrastructure/basic_manifest.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/solver_driver.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/normalizer.hpp"
#include "interfaces/i_runtime.hpp"

struct basic_runtime : i_runtime {
    basic_runtime(
        db& database,
        initial_goal_exprs& goals,
        uint32_t initial_frame_offset,
        size_t max_resolutions,
        uint32_t random_seed = 0);

    bool next() override;
    bool solved() const override;

    const expr* normalize(framed_expr) override;
    lemma derive_decision_lemma() const override;
    lemma derive_resolution_lemma() const override;

private:
    basic_manifest manifest_;
    normalizer normalizer_;
    solver_driver driver_;
};

#endif
