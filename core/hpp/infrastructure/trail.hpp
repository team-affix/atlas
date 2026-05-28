#ifndef TRAIL_HPP
#define TRAIL_HPP

#include <stack>
#include "interfaces/i_push_trail_frame.hpp"
#include "interfaces/i_pop_trail_frame.hpp"
#include "interfaces/i_log_to_current_trail_frame.hpp"

struct trail
    : i_push_trail_frame
    , i_pop_trail_frame
    , i_log_to_current_trail_frame {
    void push() override;
    void pop() override;
    void log(std::unique_ptr<i_backtrackable>) override;
    size_t depth() const;
private:
    std::stack<std::unique_ptr<i_backtrackable>> undo_stack;
    std::stack<size_t>                           frame_boundary_stack;
};

#endif
