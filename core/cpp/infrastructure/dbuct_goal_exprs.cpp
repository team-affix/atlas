#include "infrastructure/dbuct_goal_exprs.hpp"

framed_expr dbuct_goal_exprs::get(const goal_lineage* gl) const {
    return exprs_.at(gl);
}

void dbuct_goal_exprs::set(const goal_lineage* gl, framed_expr fe) {
    auto [_, inserted] = exprs_.insert({gl, std::move(fe)});
    DEBUG_ASSERT(inserted);
    log(goal_expr_insert{gl, exprs_.at(gl)});
}

void dbuct_goal_exprs::unset(const goal_lineage* gl) {
    framed_expr captured = std::move(exprs_.at(gl));
    exprs_.erase(gl);
    log(goal_expr_erase{gl, std::move(captured)});
}

void dbuct_goal_exprs::push_frame() {
    frame_stack_.push(frame{});
}

void dbuct_goal_exprs::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions.rbegin(); it != current.actions.rend(); ++it)
        undo_action(*it);
}

void dbuct_goal_exprs::log(goal_expr_action action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions.push_back(std::move(action));
}

void dbuct_goal_exprs::undo_action(const goal_expr_action& action) {
    if (const auto* ins = std::get_if<goal_expr_insert>(&action))
        exprs_.erase(ins->gl);
    else {
        const auto& er = std::get<goal_expr_erase>(action);
        exprs_.insert({er.gl, er.value});
    }
}
