#include "infrastructure/dbuct_frame_bump_allocator.hpp"

dbuct_frame_bump_allocator::dbuct_frame_bump_allocator(uint32_t initial)
    : next_frame_offset_(initial), frame_stack_(std::deque<frame>{frame{}}) {}

uint32_t dbuct_frame_bump_allocator::bump(uint32_t n) {
    const uint32_t base = next_frame_offset_;
    next_frame_offset_ += n;
    log(scalar_add_u32{n});
    return base;
}

uint32_t dbuct_frame_bump_allocator::peek() const { return next_frame_offset_; }

void dbuct_frame_bump_allocator::push_frame() { frame_stack_.push(frame{}); }

void dbuct_frame_bump_allocator::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions_.rbegin(); it != current.actions_.rend(); ++it)
        undo_action(*it);
}

void dbuct_frame_bump_allocator::log(frame_bump_action action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions_.push_back(std::move(action));
}

void dbuct_frame_bump_allocator::undo_action(const frame_bump_action& action) {
    const auto& add = std::get<scalar_add_u32>(action);
    next_frame_offset_ -= add.amount;
}
