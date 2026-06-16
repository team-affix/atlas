#include "infrastructure/ridge_runtime.hpp"

ridge_runtime::ridge_runtime(
    db& database,
    initial_goal_exprs& goals,
    uint32_t initial_frame_offset,
    size_t max_resolutions,
    uint32_t random_seed,
    double exploration_constant)
    : manifest_(database, goals, initial_frame_offset, max_resolutions, random_seed, exploration_constant),
      normalizer_(manifest_.loc_),
      driver_(manifest_.solver_.solve()) {}

bool ridge_runtime::next() {
    return driver_.next();
}

bool ridge_runtime::solved() const {
    return driver_.solved();
}

const expr* ridge_runtime::normalize(const expr* e) {
    return normalizer_.normalize(e);
}

lemma ridge_runtime::derive_decision_lemma() const {
    return manifest_.decision_memory_.derive_decision_lemma();
}

lemma ridge_runtime::derive_resolution_lemma() const {
    return manifest_.resolution_memory_.derive_resolution_lemma();
}
