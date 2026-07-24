#include "infrastructure/dbuct_goal_work_values.hpp"

dbuct_goal_work_values::dbuct_goal_work_values()
    : frame_stack_(std::deque<frame>{frame{}}) {}

double dbuct_goal_work_values::get(const goal_lineage* gl) const {
    return values_.at(gl);
}

void dbuct_goal_work_values::set(const goal_lineage* gl, double w) {
    auto [_, inserted] = values_.insert({gl, w});
    DEBUG_ASSERT(inserted);
    log(goal_work_value_insert{gl});
}

void dbuct_goal_work_values::erase(const goal_lineage* gl) {
    const auto it = values_.find(gl);
    DEBUG_ASSERT(it != values_.end());
    const double previous = it->second;
    values_.erase(it);
    log(goal_work_value_erase{gl, previous});
}

void dbuct_goal_work_values::push_frame() { frame_stack_.push(frame{}); }

void dbuct_goal_work_values::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions_.rbegin(); it != current.actions_.rend(); ++it)
        undo_action(*it);
}

void dbuct_goal_work_values::log(goal_work_value_action action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions_.push_back(std::move(action));
}

void dbuct_goal_work_values::undo_action(const goal_work_value_action& action) {
    if (const auto* ins = std::get_if<goal_work_value_insert>(&action)) {
        auto erased = values_.erase(ins->gl);
        DEBUG_ASSERT(erased == 1);
    } else {
        const auto& er = std::get<goal_work_value_erase>(action);
        auto [_, inserted] = values_.insert({er.gl, er.previous_work});
        DEBUG_ASSERT(inserted);
    }
}
