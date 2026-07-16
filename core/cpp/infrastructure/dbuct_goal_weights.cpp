#include "infrastructure/dbuct_goal_weights.hpp"

dbuct_goal_weights::dbuct_goal_weights()
    : frame_stack_(std::deque<frame>{frame{}}) {}

double dbuct_goal_weights::get(const goal_lineage* gl) const {
    return weights_.at(gl);
}

void dbuct_goal_weights::set(const goal_lineage* gl, double w) {
    auto [_, inserted] = weights_.insert({gl, w});
    DEBUG_ASSERT(inserted);
    log(goal_weight_insert{gl});
}

void dbuct_goal_weights::erase(const goal_lineage* gl) {
    const auto it = weights_.find(gl);
    DEBUG_ASSERT(it != weights_.end());
    const double previous = it->second;
    weights_.erase(it);
    log(goal_weight_erase{gl, previous});
}

void dbuct_goal_weights::push_frame() { frame_stack_.push(frame{}); }

void dbuct_goal_weights::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions_.rbegin(); it != current.actions_.rend(); ++it)
        undo_action(*it);
}

void dbuct_goal_weights::log(goal_weight_action action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions_.push_back(std::move(action));
}

void dbuct_goal_weights::undo_action(const goal_weight_action& action) {
    if (const auto* ins = std::get_if<goal_weight_insert>(&action)) {
        auto erased = weights_.erase(ins->gl);
        DEBUG_ASSERT(erased == 1);
    } else {
        const auto& er = std::get<goal_weight_erase>(action);
        auto [_, inserted] = weights_.insert({er.gl, er.previous_weight});
        DEBUG_ASSERT(inserted);
    }
}
