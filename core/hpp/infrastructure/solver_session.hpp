#ifndef SOLVER_SESSION_HPP
#define SOLVER_SESSION_HPP

#include "infrastructure/coroutine.hpp"
#include "interfaces/i_advance_solver_session.hpp"
#include "interfaces/i_derive_resolution_lemma.hpp"
#include "interfaces/i_get_solver_session_solved.hpp"
#include "interfaces/i_normalizer.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/lemma.hpp"
#include "value_objects/sim_termination.hpp"

struct solver_session
    : i_advance_solver_session
    , i_get_solver_session_solved
    , i_normalizer
    , i_derive_resolution_lemma {
    solver_session(
        coroutine<sim_termination, void> search,
        i_normalizer& norm,
        i_derive_resolution_lemma& derive_res_lemma);

    bool next() override;
    bool solved() const override;
    const expr* normalize(const expr*) override;
    lemma derive_resolution_lemma() const override;
private:
    coroutine<sim_termination, void> search_;
    i_normalizer& norm_;
    i_derive_resolution_lemma& derive_res_lemma_;
    bool solved_;
};

#endif
