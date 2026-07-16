#include "infrastructure/dbuct_cumulative_grounded_weight.hpp"

dbuct_cumulative_grounded_weight::dbuct_cumulative_grounded_weight()
    : value_(0.0)
    , frame_stack_(std::deque<frame>{frame{}}) {}

void dbuct_cumulative_grounded_weight::accumulate(double w) {
    value_ += w;
    log(scalar_add_f64{w});
}

double dbuct_cumulative_grounded_weight::get() const {
    return value_;
}

void dbuct_cumulative_grounded_weight::push_frame() { frame_stack_.push(frame{}); }

void dbuct_cumulative_grounded_weight::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions_.rbegin(); it != current.actions_.rend(); ++it)
        undo_action(*it);
}

void dbuct_cumulative_grounded_weight::log(cumulative_grounded_weight_action action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions_.push_back(std::move(action));
}

void dbuct_cumulative_grounded_weight::undo_action(const cumulative_grounded_weight_action& action) {
    const auto& add = std::get<scalar_add_f64>(action);
    value_ -= add.amount;
}
