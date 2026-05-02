#ifndef TRAIL_HPP
#define TRAIL_HPP

#include <stack>
#include "i_trail.hpp"

struct trail : i_trail {
    void push() override;
    void pop() override;
    void log(std::unique_ptr<i_backtrackable>) override;
    size_t depth() const;
private:
    std::stack<std::unique_ptr<i_backtrackable>> undo_stack;
    std::stack<size_t>                           frame_boundary_stack;
};

#endif
