#ifndef I_RUNTIME_HPP
#define I_RUNTIME_HPP

#include "interfaces/i_advance_solver_session.hpp"
#include "interfaces/i_derive_decision_lemma.hpp"
#include "interfaces/i_derive_resolution_lemma.hpp"
#include "interfaces/i_get_solver_session_solved.hpp"
#include "interfaces/i_normalizer.hpp"

struct i_runtime
    : i_advance_solver_session
    , i_get_solver_session_solved
    , i_normalizer
    , i_derive_decision_lemma
    , i_derive_resolution_lemma {
    virtual ~i_runtime() = default;
};

#endif
