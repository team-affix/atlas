#include "infrastructure/dbuct_nearest_decision.hpp"

void dbuct_nearest_decision::note_unit_resolution(const resolution_lineage* rl) {
    nd_.insert({rl, nd_.at(rl->parent->parent)});
    log(nearest_decision_action{nearest_decision_insert{rl, nd_.at(rl)}});
}

void dbuct_nearest_decision::note_decision_resolution(const resolution_lineage* rl) {
    nd_.insert({rl, rl});
    log(nearest_decision_insert{rl, rl});
}

const resolution_lineage* dbuct_nearest_decision::get_nearest_decision(const resolution_lineage* rl) const {
    return nd_.at(rl);
}

void dbuct_nearest_decision::push_frame() { frame_stack_.push(frame{}); }

void dbuct_nearest_decision::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions.rbegin(); it != current.actions.rend(); ++it)
        undo_action(*it);
}

void dbuct_nearest_decision::log(nearest_decision_action action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions.push_back(std::move(action));
}

void dbuct_nearest_decision::undo_action(const nearest_decision_action& action) {
    const auto& ins = std::get<nearest_decision_insert>(action);
    nd_.erase(ins.key);
}
