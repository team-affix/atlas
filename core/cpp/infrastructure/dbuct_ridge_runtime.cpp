#include "infrastructure/dbuct_ridge_runtime.hpp"

dbuct_ridge_runtime::dbuct_ridge_runtime(
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

bool dbuct_ridge_runtime::next() {
    return driver_.next();
}

bool dbuct_ridge_runtime::solved() const {
    return driver_.solved();
}

const expr* dbuct_ridge_runtime::normalize(framed_expr fe) {
    return normalizer_.normalize(fe);
}

size_t dbuct_ridge_runtime::resolution_depth() const {
    return manifest_.resolution_memory_.get_resolution_count();
}

size_t dbuct_ridge_runtime::decision_depth() const {
    return manifest_.decision_memory_.count();
}

lemma dbuct_ridge_runtime::derive_decision_lemma() const {
    return manifest_.decision_memory_.derive_decision_lemma();
}

lemma dbuct_ridge_runtime::derive_resolution_lemma() const {
    return manifest_.resolution_memory_.derive_resolution_lemma();
}
