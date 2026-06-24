#include "infrastructure/genius_runtime.hpp"

genius_runtime::genius_runtime(
    db& database,
    initial_goal_exprs& goals,
    uint32_t initial_frame_offset,
    size_t max_resolutions,
    uint32_t random_seed,
    double exploration_constant)
    : manifest_(database, goals, initial_frame_offset, max_resolutions, random_seed, exploration_constant),
      normalizer_(manifest_.globalizer_, manifest_.expr_pool_, manifest_.expr_pool_, manifest_.bind_map_),
      driver_(manifest_.solver_.solve()) {}

bool genius_runtime::next() {
    return driver_.next();
}

bool genius_runtime::solved() const {
    return driver_.solved();
}

const expr* genius_runtime::normalize(framed_expr fe) {
    return normalizer_.normalize(fe);
}

size_t genius_runtime::resolution_depth() const {
    return manifest_.resolution_memory_.get_resolution_count();
}

size_t genius_runtime::decision_depth() const {
    return manifest_.decision_memory_.count();
}

double genius_runtime::cgw() const {
    return manifest_.cumulative_grounded_weight_.get();
}

lemma genius_runtime::derive_decision_lemma() const {
    return manifest_.decision_memory_.derive_decision_lemma();
}

lemma genius_runtime::derive_resolution_lemma() const {
    return manifest_.resolution_memory_.derive_resolution_lemma();
}
