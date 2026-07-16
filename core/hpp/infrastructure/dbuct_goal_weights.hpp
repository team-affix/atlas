#ifndef DBUCT_GOAL_WEIGHTS_HPP
#define DBUCT_GOAL_WEIGHTS_HPP

#include <deque>
#include <list>
#include <stack>
#include <unordered_map>
#include "value_objects/goal_weight_action.hpp"
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

struct dbuct_goal_weights {
    dbuct_goal_weights();
    double get(const goal_lineage*) const;
    void set(const goal_lineage*, double);
    void erase(const goal_lineage*);

    void push_frame();
    void pop_frame();

private:
    struct frame {
        std::list<goal_weight_action> actions_;
    };

    void log(goal_weight_action action);
    void undo_action(const goal_weight_action& action);

    std::unordered_map<const goal_lineage*, double> weights_;
    std::stack<frame> frame_stack_;
};

#endif
