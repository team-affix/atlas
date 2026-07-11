#include "infrastructure/dbuct_resolution_memory.hpp"

void dbuct_resolution_memory::record_resolution(const resolution_lineage* rl) {
    if (resolutions_.contains(rl))
        return;
    resolutions_.insert(rl);
    log(resolution_set_insert{rl});
}

size_t dbuct_resolution_memory::get_resolution_count() const { return resolutions_.size(); }

lemma dbuct_resolution_memory::derive_resolution_lemma() const { return lemma{resolutions_}; }

void dbuct_resolution_memory::push_frame() { frame_stack_.push(frame{}); }

void dbuct_resolution_memory::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions.rbegin(); it != current.actions.rend(); ++it)
        undo_action(*it);
}

void dbuct_resolution_memory::log(decision_memory_action action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions.push_back(std::move(action));
}

void dbuct_resolution_memory::undo_action(const decision_memory_action& action) {
    const auto& ins = std::get<resolution_set_insert>(action);
    resolutions_.erase(ins.rl);
}
