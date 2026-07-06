#include "infrastructure/dbuct_runtime.hpp"

dbuct_runtime::dbuct_runtime(
    db& database,
    initial_goal_exprs& goals,
    uint32_t initial_frame_offset,
    size_t max_resolutions,
    uint32_t random_seed,
    double exploration_constant,
    size_t grant_increment_interval)
    : manifest_(database, goals, initial_frame_offset, max_resolutions,
                random_seed, exploration_constant, grant_increment_interval),
      normalizer_(manifest_.globalizer_, manifest_.expr_pool_,
                  manifest_.expr_pool_, manifest_.bind_map_),
      driver_(manifest_.solver_.solve()) {}

bool dbuct_runtime::next() {
    return driver_.next();
}

bool dbuct_runtime::solved() const {
    return driver_.solved();
}

const expr* dbuct_runtime::normalize(framed_expr fe) {
    return normalizer_.normalize(fe);
}

size_t dbuct_runtime::resolution_depth() const {
    return manifest_.resolution_memory_.get_resolution_count();
}

size_t dbuct_runtime::decision_depth() const {
    return manifest_.decision_memory_.count();
}

lemma dbuct_runtime::derive_decision_lemma() const {
    return manifest_.decision_memory_.derive_decision_lemma();
}

lemma dbuct_runtime::derive_resolution_lemma() const {
    return manifest_.resolution_memory_.derive_resolution_lemma();
}
