#ifndef BASIC_SOLVER_SESSION_HPP
#define BASIC_SOLVER_SESSION_HPP

#include <cstdint>
#include "infrastructure/basic_manifest.hpp"
#include "infrastructure/coroutine.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/normalizer.hpp"
#include "interfaces/i_derive_decision_lemma.hpp"
#include "interfaces/i_derive_resolution_lemma.hpp"
#include "interfaces/i_normalizer.hpp"
#include "value_objects/sim_termination.hpp"

struct basic_solver_session
    : i_normalizer
    , i_derive_decision_lemma
    , i_derive_resolution_lemma {
    basic_solver_session(
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
    coroutine<sim_termination, void> search_;
    bool solved_;
};

#endif
