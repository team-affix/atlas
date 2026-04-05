#ifndef TRAIL_HPP
#define TRAIL_HPP

#include <stack>
#include <functional>
#include <list>

struct action {
    std::function<void()> undo;
    std::function<void()> redo;
};

struct frame {
    std::list<action> actions;
    std::list<frame> children;
};

struct trail {
    void push();
    void pop();
    std::list<frame>::iterator undo();
    void redo(std::list<frame>::iterator);
    void log(const std::function<void()>&, const std::function<void()>&);
    size_t depth() const;
#ifndef DEBUG
private:
#endif
    frame& current();
    frame root;
    std::stack<std::list<frame>::iterator> path;
};

#endif
