#include "infrastructure/ridge_bt_fgt_runtime.hpp"

ridge_bt_fgt_runtime::ridge_bt_fgt_runtime(
    db& database,
    initial_goal_exprs& goals,
    uint32_t initial_frame_offset,
    size_t max_resolutions,
    uint32_t random_seed,
    double exploration_constant,
    size_t max_clauses)
    : manifest_(database, goals, initial_frame_offset, max_resolutions, random_seed,
                exploration_constant, max_clauses) {}

bool ridge_bt_fgt_runtime::next() {
    return manifest_.driver_.next();
}

bool ridge_bt_fgt_runtime::solved() const {
    return manifest_.driver_.solved();
}

const expr* ridge_bt_fgt_runtime::normalize(framed_expr fe) {
    return manifest_.normalizer_.normalize(fe);
}

size_t ridge_bt_fgt_runtime::resolution_depth() const {
    return manifest_.resolution_memory_.get_resolution_count();
}

size_t ridge_bt_fgt_runtime::decision_depth() const {
    return manifest_.decision_memory_.count();
}

lemma ridge_bt_fgt_runtime::derive_decision_lemma() const {
    return manifest_.decision_memory_.derive_decision_lemma();
}

lemma ridge_bt_fgt_runtime::derive_resolution_lemma() const {
    return manifest_.resolution_memory_.derive_resolution_lemma();
}
