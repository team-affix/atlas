#ifndef DBUCT_GOAL_EXPRS_HPP
#define DBUCT_GOAL_EXPRS_HPP

#include <deque>
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

private:
    struct frame {
        std::list<goal_expr_action> actions;
    };

    using map_t = std::unordered_map<const goal_lineage*, framed_expr>;

    void log(goal_expr_action action);
    void undo_action(const goal_expr_action& action);

    map_t exprs_;
    std::stack<frame> frame_stack_{std::deque<frame>{frame{}}};
};

#endif
