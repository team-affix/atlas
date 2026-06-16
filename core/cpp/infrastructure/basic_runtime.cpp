#include "infrastructure/basic_runtime.hpp"

basic_runtime::basic_runtime(
    db& database,
    initial_goal_exprs& goals,
    uint32_t initial_frame_offset,
    size_t max_resolutions,
    uint32_t random_seed)
    : manifest_(database, goals, initial_frame_offset, max_resolutions, random_seed),
      normalizer_(manifest_.loc_),
      driver_(manifest_.solver_.solve()) {}

bool basic_runtime::next() {
    return driver_.next();
}

bool basic_runtime::solved() const {
    return driver_.solved();
}

const expr* basic_runtime::normalize(const expr* e) {
    return normalizer_.normalize(e);
}

lemma basic_runtime::derive_decision_lemma() const {
    return manifest_.decision_memory_.derive_decision_lemma();
}

lemma basic_runtime::derive_resolution_lemma() const {
    return manifest_.resolution_memory_.derive_resolution_lemma();
}
