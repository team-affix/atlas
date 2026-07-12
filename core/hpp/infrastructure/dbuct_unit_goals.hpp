#ifndef DBUCT_UNIT_GOALS_HPP
#define DBUCT_UNIT_GOALS_HPP

#include <deque>
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
    std::stack<frame> frame_stack_{std::deque<frame>{frame{}}};
};

#endif
