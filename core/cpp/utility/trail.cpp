#include "../../hpp/utility/trail.hpp"

void trail::push() {
    frame_boundary_stack.push(undo_stack.size());
}

void trail::pop() {
    // get the last frame boundary
    auto checkpoint = frame_boundary_stack.top();
    // pop the frame boundary
    frame_boundary_stack.pop();
    // pop the undo stack up to the last frame boundary
    while (undo_stack.size() > checkpoint) {
        // execute the undo function
        undo_stack.top()->backtrack();
        // pop the undo function
        undo_stack.pop();
    }
}

void trail::log(std::unique_ptr<i_backtrackable> op) {
    undo_stack.push(std::move(op));
}

size_t trail::depth() const {
    return frame_boundary_stack.size();
}
