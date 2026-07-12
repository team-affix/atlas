#ifndef DBUCT_FRAME_BUMP_ALLOCATOR_HPP
#define DBUCT_FRAME_BUMP_ALLOCATOR_HPP

#include <cstdint>
#include <deque>
#include <list>
#include <stack>
#include "value_objects/frame_bump_action.hpp"
#include "debug_assert.hpp"

struct dbuct_frame_bump_allocator {
    dbuct_frame_bump_allocator(uint32_t initial);

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
    std::stack<frame> frame_stack_{std::deque<frame>{frame{}}};
};

#endif
