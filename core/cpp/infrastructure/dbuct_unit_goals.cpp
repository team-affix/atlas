#include "infrastructure/dbuct_unit_goals.hpp"

void dbuct_unit_goals::push(const goal_lineage* gl) {
    queue_.push_back(gl);
    log(vector_push_back_goal{gl});
}

std::optional<const goal_lineage*> dbuct_unit_goals::pop() {
    if (queue_.empty())
        return std::nullopt;
    const goal_lineage* gl = queue_.back();
    queue_.pop_back();
    log(vector_pop_back_goal{gl});
    return gl;
}

void dbuct_unit_goals::push_frame() { frame_stack_.push(frame{}); }

void dbuct_unit_goals::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions.rbegin(); it != current.actions.rend(); ++it)
        undo_action(*it);
}

void dbuct_unit_goals::log(unit_goals_action action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions.push_back(std::move(action));
}

void dbuct_unit_goals::undo_action(const unit_goals_action& action) {
    if (const auto* push = std::get_if<vector_push_back_goal>(&action))
        queue_.pop_back();
    else {
        const auto& pop = std::get<vector_pop_back_goal>(action);
        queue_.push_back(pop.gl);
    }
}
