#ifndef TRAIL_HPP
#define TRAIL_HPP

#include <memory>
#include <stack>
#include "interfaces/i_backtrackable.hpp"
#include "interfaces/i_log_to_current_trail_frame.hpp"

struct trail : i_log_to_current_trail_frame {
    void push();
    void pop();
    void log(std::unique_ptr<i_backtrackable>) override;
    size_t depth() const;
private:
    std::stack<std::unique_ptr<i_backtrackable>> undo_stack;
    std::stack<size_t>                           frame_boundary_stack;
};

inline void trail::push() {
    frame_boundary_stack.push(undo_stack.size());
}

inline void trail::pop() {
    auto checkpoint = frame_boundary_stack.top();
    frame_boundary_stack.pop();
    while (undo_stack.size() > checkpoint) {
        undo_stack.top()->backtrack();
        undo_stack.pop();
    }
}

inline void trail::log(std::unique_ptr<i_backtrackable> op) {
    undo_stack.push(std::move(op));
}

inline size_t trail::depth() const {
    return frame_boundary_stack.size();
}

#endif
