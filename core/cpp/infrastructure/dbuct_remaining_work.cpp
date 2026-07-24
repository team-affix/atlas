#include "infrastructure/dbuct_remaining_work.hpp"

dbuct_remaining_work::dbuct_remaining_work()
    : value_(0.0)
    , frame_stack_(std::deque<frame>{frame{}}) {}

void dbuct_remaining_work::add(double w) {
    value_ += w;
    log(scalar_add_f64{w});
}

void dbuct_remaining_work::subtract(double w) {
    value_ -= w;
    log(scalar_add_f64{-w});
}

double dbuct_remaining_work::get() const {
    return value_;
}

void dbuct_remaining_work::push_frame() { frame_stack_.push(frame{}); }

void dbuct_remaining_work::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions_.rbegin(); it != current.actions_.rend(); ++it)
        undo_action(*it);
}

void dbuct_remaining_work::log(remaining_work_action action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions_.push_back(std::move(action));
}

void dbuct_remaining_work::undo_action(const remaining_work_action& action) {
    const auto& add = std::get<scalar_add_f64>(action);
    value_ -= add.amount;
}
