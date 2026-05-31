#include "infrastructure/basic_solver_session.hpp"

basic_solver_session::basic_solver_session(
    db& database,
    initial_goal_exprs& goals,
    size_t max_resolutions,
    uint32_t random_seed)
    : manifest_(database, goals, max_resolutions, random_seed),
      normalizer_(manifest_.loc_),
      search_(manifest_.solver_.solve()) {}

bool basic_solver_session::next() {
    if (search_.done())
        return false;

    search_.resume();

    if (!search_.has_yield())
        return false;

    solved_ = search_.consume_yield() == sim_termination::solved;
    
    return true;
}

bool basic_solver_session::solved() const {
    return solved_;
}

const expr* basic_solver_session::normalize(const expr* e) {
    return normalizer_.normalize(e);
}

lemma basic_solver_session::derive_decision_lemma() const {
    return manifest_.decision_memory_.derive_decision_lemma();
}

lemma basic_solver_session::derive_resolution_lemma() const {
    return manifest_.resolution_memory_.derive_resolution_lemma();
}
