#include "infrastructure/quell_runtime.hpp"

quell_runtime::quell_runtime(
    db& database,
    initial_goal_exprs& goals,
    uint32_t initial_frame_offset,
    size_t max_resolutions,
    uint32_t random_seed,
    double exploration_constant,
    double work_decay_k,
    double work_decay_j)
    : manifest_(database, goals, initial_frame_offset, max_resolutions, random_seed,
                exploration_constant, work_decay_k, work_decay_j) {}

bool quell_runtime::next() {
    return manifest_.driver_.next();
}

bool quell_runtime::solved() const {
    return manifest_.driver_.solved();
}

const expr* quell_runtime::normalize(framed_expr fe) {
    return manifest_.normalizer_.normalize(fe);
}

size_t quell_runtime::resolution_depth() const {
    return manifest_.resolution_memory_.get_resolution_count();
}

size_t quell_runtime::decision_depth() const {
    return manifest_.decision_memory_.count();
}

double quell_runtime::remaining_work() const {
    return manifest_.remaining_work_.get();
}

size_t quell_runtime::remaining_active_goals() const {
    return manifest_.srt_active_goals_.active_goals_size();
}

lemma quell_runtime::derive_decision_lemma() const {
    return manifest_.decision_memory_.derive_decision_lemma();
}

lemma quell_runtime::derive_resolution_lemma() const {
    return manifest_.resolution_memory_.derive_resolution_lemma();
}
