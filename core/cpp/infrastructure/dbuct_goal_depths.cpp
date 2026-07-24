#include "infrastructure/dbuct_goal_depths.hpp"

dbuct_goal_depths::dbuct_goal_depths()
    : frame_stack_(std::deque<frame>{frame{}}) {}

size_t dbuct_goal_depths::get(const goal_lineage* gl) const {
    return depths_.at(gl);
}

void dbuct_goal_depths::set(const goal_lineage* gl, size_t depth) {
    auto [_, inserted] = depths_.insert({gl, depth});
    DEBUG_ASSERT(inserted);
    log(goal_depth_insert{gl});
}

void dbuct_goal_depths::erase(const goal_lineage* gl) {
    const auto it = depths_.find(gl);
    DEBUG_ASSERT(it != depths_.end());
    const size_t previous = it->second;
    depths_.erase(it);
    log(goal_depth_erase{gl, previous});
}

void dbuct_goal_depths::push_frame() { frame_stack_.push(frame{}); }

void dbuct_goal_depths::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions_.rbegin(); it != current.actions_.rend(); ++it)
        undo_action(*it);
}

void dbuct_goal_depths::log(goal_depth_action action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions_.push_back(std::move(action));
}

void dbuct_goal_depths::undo_action(const goal_depth_action& action) {
    if (const auto* ins = std::get_if<goal_depth_insert>(&action)) {
        auto erased = depths_.erase(ins->gl);
        DEBUG_ASSERT(erased == 1);
    } else {
        const auto& er = std::get<goal_depth_erase>(action);
        auto [_, inserted] = depths_.insert({er.gl, er.previous_depth});
        DEBUG_ASSERT(inserted);
    }
}
