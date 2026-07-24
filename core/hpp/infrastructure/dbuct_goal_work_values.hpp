#ifndef DBUCT_GOAL_WORK_VALUES_HPP
#define DBUCT_GOAL_WORK_VALUES_HPP

#include <deque>
#include <list>
#include <stack>
#include <unordered_map>
#include "value_objects/goal_work_value_action.hpp"
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

struct dbuct_goal_work_values {
    dbuct_goal_work_values();
    double get(const goal_lineage*) const;
    void set(const goal_lineage*, double);
    void erase(const goal_lineage*);

    void push_frame();
    void pop_frame();

private:
    struct frame {
        std::list<goal_work_value_action> actions_;
    };

    void log(goal_work_value_action action);
    void undo_action(const goal_work_value_action& action);

    std::unordered_map<const goal_lineage*, double> values_;
    std::stack<frame> frame_stack_;
};

#endif
