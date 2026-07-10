#ifndef DBUCT_GOAL_EXPRS_HPP
#define DBUCT_GOAL_EXPRS_HPP

#include <list>
#include <stack>
#include <unordered_map>
#include "value_objects/framed_expr.hpp"
#include "value_objects/goal_expr_action.hpp"
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

struct dbuct_goal_exprs {
    framed_expr get(const goal_lineage* gl) const;
    void set(const goal_lineage* gl, framed_expr fe);
    void unset(const goal_lineage* gl);

    void push_frame();
    void pop_frame();
    void squash_frame();

private:
    struct frame {
        std::list<goal_expr_action> actions;
    };

    using map_t = std::unordered_map<const goal_lineage*, framed_expr>;

    void log(goal_expr_action action);
    void undo_action(const goal_expr_action& action);

    map_t exprs_;
    std::stack<frame> frame_stack_;
};

inline framed_expr dbuct_goal_exprs::get(const goal_lineage* gl) const {
    return exprs_.at(gl);
}

inline void dbuct_goal_exprs::set(const goal_lineage* gl, framed_expr fe) {
    auto [_, inserted] = exprs_.insert({gl, std::move(fe)});
    DEBUG_ASSERT(inserted);
    log(goal_expr_insert{gl, exprs_.at(gl)});
}

inline void dbuct_goal_exprs::unset(const goal_lineage* gl) {
    framed_expr captured = std::move(exprs_.at(gl));
    exprs_.erase(gl);
    log(goal_expr_erase{gl, std::move(captured)});
}

inline void dbuct_goal_exprs::push_frame() {
    frame_stack_.push(frame{});
}

inline void dbuct_goal_exprs::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions.rbegin(); it != current.actions.rend(); ++it)
        undo_action(*it);
}

inline void dbuct_goal_exprs::squash_frame() {
    auto top = std::move(frame_stack_.top());
    frame_stack_.pop();
    auto& parent = frame_stack_.top().actions;
    parent.splice(parent.end(), std::move(top.actions));
}

inline void dbuct_goal_exprs::log(goal_expr_action action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions.push_back(std::move(action));
}

inline void dbuct_goal_exprs::undo_action(const goal_expr_action& action) {
    if (const auto* ins = std::get_if<goal_expr_insert>(&action))
        exprs_.erase(ins->gl);
    else {
        const auto& er = std::get<goal_expr_erase>(action);
        exprs_.insert({er.gl, er.value});
    }
}

#endif
