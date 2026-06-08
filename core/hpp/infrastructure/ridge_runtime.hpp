#ifndef RIDGE_RUNTIME_HPP
#define RIDGE_RUNTIME_HPP

#include <cstdint>
#include "infrastructure/ridge_manifest.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/solver_driver.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/normalizer.hpp"
#include "interfaces/i_derive_decision_lemma.hpp"
#include "interfaces/i_derive_resolution_lemma.hpp"
#include "interfaces/i_normalizer.hpp"

struct ridge_runtime
    : i_normalizer
    , i_derive_decision_lemma
    , i_derive_resolution_lemma {
    ridge_runtime(
        db& database,
        initial_goal_exprs& goals,
        size_t initial_var_count,
        size_t max_resolutions,
        uint32_t random_seed,
        double exploration_constant);

    bool next();
    bool solved() const;

    const expr* normalize(const expr*) override;
    lemma derive_decision_lemma() const override;
    lemma derive_resolution_lemma() const override;

private:
    ridge_manifest manifest_;
    normalizer normalizer_;
    solver_driver driver_;
};

#endif
