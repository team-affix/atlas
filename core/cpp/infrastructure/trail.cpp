#include "infrastructure/trail.hpp"

void trail::push() {
    frame_boundary_stack.push(undo_stack.size());
}

void trail::pop() {
    auto checkpoint = frame_boundary_stack.top();
    frame_boundary_stack.pop();
    while (undo_stack.size() > checkpoint) {
        undo_stack.top()->backtrack();
        undo_stack.pop();
    }
}

void trail::log(std::unique_ptr<i_backtrackable> op) {
    undo_stack.push(std::move(op));
}

size_t trail::depth() const {
    return frame_boundary_stack.size();
}
