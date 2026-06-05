#ifndef BASIC_RUNTIME_HPP
#define BASIC_RUNTIME_HPP

#include <cstdint>
#include "infrastructure/basic_manifest.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/solver_driver.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/normalizer.hpp"
#include "interfaces/i_derive_decision_lemma.hpp"
#include "interfaces/i_derive_resolution_lemma.hpp"
#include "interfaces/i_normalizer.hpp"

struct basic_runtime
    : i_normalizer
    , i_derive_decision_lemma
    , i_derive_resolution_lemma {
    basic_runtime(
        db& database,
        initial_goal_exprs& goals,
        size_t initial_var_count,
        size_t max_resolutions,
        uint32_t random_seed = 0);

    bool next();
    bool solved() const;

    const expr* normalize(const expr*) override;
    lemma derive_decision_lemma() const override;
    lemma derive_resolution_lemma() const override;

private:
    basic_manifest manifest_;
    normalizer normalizer_;
    solver_driver driver_;
};

#endif
