#include "infrastructure/dbuct_decision_memory.hpp"

void dbuct_decision_memory::record_decision(const resolution_lineage* rl) {
    if (decisions_.contains(rl))
        return;
    decisions_.insert(rl);
    log(resolution_set_insert{rl});
}

size_t dbuct_decision_memory::count() const { return decisions_.size(); }

lemma dbuct_decision_memory::derive_decision_lemma() const { return lemma{decisions_}; }

void dbuct_decision_memory::push_frame() { frame_stack_.push(frame{}); }

void dbuct_decision_memory::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions.rbegin(); it != current.actions.rend(); ++it)
        undo_action(*it);
}

void dbuct_decision_memory::log(decision_memory_action action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions.push_back(std::move(action));
}

void dbuct_decision_memory::undo_action(const decision_memory_action& action) {
    const auto& ins = std::get<resolution_set_insert>(action);
    decisions_.erase(ins.rl);
}
