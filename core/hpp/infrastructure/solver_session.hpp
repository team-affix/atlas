#ifndef SOLVER_SESSION_HPP
#define SOLVER_SESSION_HPP

#include "infrastructure/coroutine.hpp"
#include "interfaces/i_derive_resolution_lemma.hpp"
#include "interfaces/i_normalizer.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/lemma.hpp"
#include "value_objects/sim_termination.hpp"

struct solver_session {
    solver_session(
        coroutine<sim_termination, void> search,
        i_normalizer& norm,
        i_derive_resolution_lemma& derive_res_lemma);

    bool next();
    bool solved() const;

    const expr* normalize(const expr*);
    lemma derive_resolution_lemma() const;
private:
    coroutine<sim_termination, void> search_;
    i_normalizer& norm_;
    i_derive_resolution_lemma& derive_res_lemma_;
    bool solved_;
};

#endif
