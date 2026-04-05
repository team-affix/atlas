#include "../hpp/trail.hpp"

void trail::push() {
    frame& c = current();
    c.children.emplace_front();
    path.push(c.children.begin());
}

void trail::pop() {
    auto it = undo();
    current().children.erase(it);
}

std::list<frame>::iterator trail::undo() {
    auto result = path.top();
    for (
        auto it = result->actions.rbegin();
        it != result->actions.rend();
        ++it)
        it->undo();
    path.pop();
    return result;
}

void trail::redo(std::list<frame>::iterator frame) {
    for (
        auto it = frame->actions.begin();
        it != frame->actions.end();
        ++it)
        it->redo();
    path.push(frame);
}

void trail::log(const std::function<void()>& undo, const std::function<void()>& redo) {
    current().actions.emplace_back(action{undo, redo});
}

size_t trail::depth() const {
    return path.size();
}

frame& trail::current() {
    if (path.empty())
        return root;
    return *path.top();
}
