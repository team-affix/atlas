#include "infrastructure/dbuct_horizon_runtime.hpp"

dbuct_horizon_runtime::dbuct_horizon_runtime(
    db& database,
    initial_goal_exprs& goals,
    uint32_t initial_frame_offset,
    size_t max_resolutions,
    uint32_t random_seed,
    double exploration_constant,
    size_t grant_increment_interval)
    : manifest_(database, goals, initial_frame_offset, max_resolutions,
                random_seed, exploration_constant, grant_increment_interval) {}

bool dbuct_horizon_runtime::next() {
    return manifest_.driver_.next();
}

bool dbuct_horizon_runtime::solved() const {
    return manifest_.driver_.solved();
}

const expr* dbuct_horizon_runtime::normalize(framed_expr fe) {
    return manifest_.normalizer_.normalize(fe);
}

size_t dbuct_horizon_runtime::resolution_depth() const {
    return manifest_.resolution_memory_.get_resolution_count();
}

size_t dbuct_horizon_runtime::decision_depth() const {
    return manifest_.decision_memory_.count();
}

double dbuct_horizon_runtime::cgw() const {
    return manifest_.cumulative_grounded_weight_.get();
}

lemma dbuct_horizon_runtime::derive_decision_lemma() const {
    return manifest_.decision_memory_.derive_decision_lemma();
}

lemma dbuct_horizon_runtime::derive_resolution_lemma() const {
    return manifest_.resolution_memory_.derive_resolution_lemma();
}
