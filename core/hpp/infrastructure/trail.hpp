#ifndef TRAIL_HPP
#define TRAIL_HPP

#include <memory>
#include <stack>
#include "interfaces/i_backtrackable.hpp"

struct trail {
    void push();
    void pop();
    void squash_one();
    void log(std::unique_ptr<i_backtrackable>);
    size_t depth() const;
private:
    std::stack<std::unique_ptr<i_backtrackable>> undo_stack;
    std::stack<size_t>                           frame_boundary_stack;
};

#endif
