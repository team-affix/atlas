#include "infrastructure/ridge_fc_runtime.hpp"

ridge_fc_runtime::ridge_fc_runtime(
    db& database,
    initial_goal_exprs& goals,
    uint32_t initial_frame_offset,
    size_t max_resolutions,
    uint32_t random_seed,
    double exploration_constant)
    : manifest_(database, goals, initial_frame_offset, max_resolutions, random_seed, exploration_constant) {}

bool ridge_fc_runtime::next() {
    return manifest_.driver_.next();
}

bool ridge_fc_runtime::solved() const {
    return manifest_.driver_.solved();
}

const expr* ridge_fc_runtime::normalize(framed_expr fe) {
    return manifest_.normalizer_.normalize(fe);
}

size_t ridge_fc_runtime::resolution_depth() const {
    return manifest_.resolution_memory_.get_resolution_count();
}

size_t ridge_fc_runtime::decision_depth() const {
    return manifest_.decision_memory_.count();
}

lemma ridge_fc_runtime::derive_decision_lemma() const {
    return manifest_.decision_memory_.derive_decision_lemma();
}

lemma ridge_fc_runtime::derive_resolution_lemma() const {
    return manifest_.resolution_memory_.derive_resolution_lemma();
}
