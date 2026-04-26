#ifndef TRAIL_HPP
#define TRAIL_HPP

#include <stack>
#include <functional>

struct trail {
    void push();
    void pop();
    void log(const std::function<void()>&);
    size_t depth() const;
#ifndef DEBUG
private:
#endif
    std::stack<std::function<void()>> undo_stack;
    std::stack<size_t>                frame_boundary_stack;
};

#endif
