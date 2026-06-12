#ifndef HORIZON_RUNTIME_HPP
#define HORIZON_RUNTIME_HPP

#include <cstdint>
#include "infrastructure/horizon_manifest.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/solver_driver.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/normalizer.hpp"
#include "interfaces/i_runtime.hpp"

struct horizon_runtime : i_runtime {
    horizon_runtime(
        db& database,
        initial_goal_exprs& goals,
        size_t initial_var_count,
        size_t max_resolutions,
        uint32_t random_seed,
        double exploration_constant);

    bool next() override;
    bool solved() const override;

    const expr* normalize(const expr*) override;
    lemma derive_decision_lemma() const override;
    lemma derive_resolution_lemma() const override;

private:
    horizon_manifest manifest_;
    normalizer normalizer_;
    solver_driver driver_;
};

#endif
