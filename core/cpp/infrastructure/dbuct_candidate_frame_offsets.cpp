#include "infrastructure/dbuct_candidate_frame_offsets.hpp"

void dbuct_candidate_frame_offsets::set(const resolution_lineage* rl, uint32_t frame_offset) {
    offsets_.insert({rl, frame_offset});
    log(candidate_offset_insert{rl, frame_offset});
}

void dbuct_candidate_frame_offsets::unset(const resolution_lineage* rl) {
    uint32_t captured = offsets_.at(rl);
    offsets_.erase(rl);
    log(candidate_offset_erase{rl, captured});
}

uint32_t dbuct_candidate_frame_offsets::get(const resolution_lineage* rl) const {
    return offsets_.at(rl);
}

void dbuct_candidate_frame_offsets::push_frame() { frame_stack_.push(frame{}); }

void dbuct_candidate_frame_offsets::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions.rbegin(); it != current.actions.rend(); ++it)
        undo_action(*it);
}

void dbuct_candidate_frame_offsets::log(candidate_offset_action action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions.push_back(std::move(action));
}

void dbuct_candidate_frame_offsets::undo_action(const candidate_offset_action& action) {
    if (const auto* ins = std::get_if<candidate_offset_insert>(&action))
        offsets_.erase(ins->rl);
    else {
        const auto& er = std::get<candidate_offset_erase>(action);
        offsets_.insert({er.rl, er.frame_offset});
    }
}
