#ifndef TRAIL_HPP
#define TRAIL_HPP

#include <memory>
#include <stack>
#include "interfaces/i_backtrackable.hpp"

struct trail {
    void push();
    void pop();
    // Merge the top frame into the frame below it: drops the top boundary marker
    // while keeping all its undo records (which now belong to the lower frame).
    // Unlike pop(), nothing is unwound. Used to make a pushed savepoint's changes
    // permanent within the enclosing frame once an operation commits.
    void squash_one();
    void log(std::unique_ptr<i_backtrackable>);
    size_t depth() const;
private:
    std::stack<std::unique_ptr<i_backtrackable>> undo_stack;
    std::stack<size_t>                           frame_boundary_stack;
};

#endif
