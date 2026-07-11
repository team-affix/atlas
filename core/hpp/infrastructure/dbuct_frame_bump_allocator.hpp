#ifndef DBUCT_FRAME_BUMP_ALLOCATOR_HPP
#define DBUCT_FRAME_BUMP_ALLOCATOR_HPP

#include <cstdint>
#include <list>
#include <stack>
#include "value_objects/frame_bump_action.hpp"
#include "debug_assert.hpp"

struct dbuct_frame_bump_allocator {
    explicit dbuct_frame_bump_allocator(uint32_t initial);

    uint32_t bump(uint32_t n);
    uint32_t peek() const;

    void push_frame();
    void pop_frame();

private:
    struct frame {
        std::list<frame_bump_action> actions;
    };

    void log(frame_bump_action action);
    void undo_action(const frame_bump_action& action);

    uint32_t next_frame_offset_;
    std::stack<frame> frame_stack_;
};

inline dbuct_frame_bump_allocator::dbuct_frame_bump_allocator(uint32_t initial)
    : next_frame_offset_(initial) {}

inline uint32_t dbuct_frame_bump_allocator::bump(uint32_t n) {
    const uint32_t base = next_frame_offset_;
    next_frame_offset_ += n;
    log(scalar_add_u32{n});
    return base;
}

inline uint32_t dbuct_frame_bump_allocator::peek() const { return next_frame_offset_; }

inline void dbuct_frame_bump_allocator::push_frame() { frame_stack_.push(frame{}); }

inline void dbuct_frame_bump_allocator::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions.rbegin(); it != current.actions.rend(); ++it)
        undo_action(*it);
}

inline void dbuct_frame_bump_allocator::log(frame_bump_action action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions.push_back(std::move(action));
}

inline void dbuct_frame_bump_allocator::undo_action(const frame_bump_action& action) {
    const auto& add = std::get<scalar_add_u32>(action);
    next_frame_offset_ -= add.amount;
}

#endif
