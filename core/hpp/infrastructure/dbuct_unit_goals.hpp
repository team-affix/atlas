#ifndef DBUCT_UNIT_GOALS_HPP
#define DBUCT_UNIT_GOALS_HPP

#include <list>
#include <optional>
#include <stack>
#include <vector>
#include "value_objects/lineage.hpp"
#include "value_objects/unit_goals_action.hpp"
#include "debug_assert.hpp"

struct dbuct_unit_goals {
    void push(const goal_lineage* gl);
    std::optional<const goal_lineage*> pop();

    void push_frame();
    void pop_frame();

private:
    struct frame {
        std::list<unit_goals_action> actions;
    };

    using vec_t = std::vector<const goal_lineage*>;

    void log(unit_goals_action action);
    void undo_action(const unit_goals_action& action);

    vec_t queue_;
    std::stack<frame> frame_stack_;
};

inline void dbuct_unit_goals::push(const goal_lineage* gl) {
    queue_.push_back(gl);
    log(vector_push_back_goal{gl});
}

inline std::optional<const goal_lineage*> dbuct_unit_goals::pop() {
    if (queue_.empty())
        return std::nullopt;
    const goal_lineage* gl = queue_.back();
    queue_.pop_back();
    log(vector_pop_back_goal{gl});
    return gl;
}

inline void dbuct_unit_goals::push_frame() { frame_stack_.push(frame{}); }

inline void dbuct_unit_goals::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions.rbegin(); it != current.actions.rend(); ++it)
        undo_action(*it);
}

inline void dbuct_unit_goals::log(unit_goals_action action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions.push_back(std::move(action));
}

inline void dbuct_unit_goals::undo_action(const unit_goals_action& action) {
    if (const auto* push = std::get_if<vector_push_back_goal>(&action))
        queue_.pop_back();
    else {
        const auto& pop = std::get<vector_pop_back_goal>(action);
        queue_.push_back(pop.gl);
    }
}

#endif
