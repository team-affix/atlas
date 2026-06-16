#include "infrastructure/horizon_runtime.hpp"

horizon_runtime::horizon_runtime(
    db& database,
    initial_goal_exprs& goals,
    uint32_t initial_frame_offset,
    size_t max_resolutions,
    uint32_t random_seed,
    double exploration_constant)
    : manifest_(database, goals, initial_frame_offset, max_resolutions, random_seed, exploration_constant),
      normalizer_(manifest_.loc_),
      driver_(manifest_.solver_.solve()) {}

bool horizon_runtime::next() {
    return driver_.next();
}

bool horizon_runtime::solved() const {
    return driver_.solved();
}

const expr* horizon_runtime::normalize(const expr* e) {
    return normalizer_.normalize(e);
}

lemma horizon_runtime::derive_decision_lemma() const {
    return manifest_.decision_memory_.derive_decision_lemma();
}

lemma horizon_runtime::derive_resolution_lemma() const {
    return manifest_.resolution_memory_.derive_resolution_lemma();
}
