#ifndef SEQUENCER_HPP
#define SEQUENCER_HPP

#include <list>
#include <stack>
#include "value_objects/frame_bump_action.hpp"

template<typename IndexType>
struct sequencer {
    sequencer(IndexType initial);
    IndexType next();

    void push_frame();
    void pop_frame();

private:
    struct frame {
        std::list<frame_bump_action> actions;
    };

    void log(frame_bump_action action);
    void undo_action(const frame_bump_action& action);

    IndexType index_;
    std::stack<frame> frame_stack_;
};

template<typename IndexType>
sequencer<IndexType>::sequencer(IndexType initial) : index_(initial) {}

template<typename IndexType>
IndexType sequencer<IndexType>::next() {
    IndexType result = index_;
    index_ += 1;
    log(scalar_add_u32{1});
    return result;
}

template<typename IndexType>
void sequencer<IndexType>::push_frame() {
    frame_stack_.push(frame{});
}

template<typename IndexType>
void sequencer<IndexType>::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions.rbegin(); it != current.actions.rend(); ++it)
        undo_action(*it);
}

template<typename IndexType>
void sequencer<IndexType>::log(frame_bump_action action) {
    if (!frame_stack_.empty())
        frame_stack_.top().actions.push_back(std::move(action));
}

template<typename IndexType>
void sequencer<IndexType>::undo_action(const frame_bump_action& action) {
    const auto& add = std::get<scalar_add_u32>(action);
    index_ -= add.amount;
}

#endif
