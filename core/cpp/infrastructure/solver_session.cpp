#include "infrastructure/solver_session.hpp"

solver_session::solver_session(
    coroutine<sim_termination, void> search,
    i_normalizer& norm,
    i_derive_resolution_lemma& derive_res_lemma)
    : search_(std::move(search)),
      norm_(norm),
      derive_res_lemma_(derive_res_lemma) {}

bool solver_session::next() {
    if (search_.done())
        return false;

    search_.resume();

    if (!search_.has_yield())
        return false;

    solved_ = search_.consume_yield() == sim_termination::solved;
    
    return true;
}

bool solver_session::solved() const {
    return solved_;
}

const expr* solver_session::normalize(const expr* e) {
    return norm_.normalize(e);
}

lemma solver_session::derive_resolution_lemma() const {
    return derive_res_lemma_.derive_resolution_lemma();
}
