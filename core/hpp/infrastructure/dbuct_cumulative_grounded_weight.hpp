#ifndef DBUCT_CUMULATIVE_GROUNDED_WEIGHT_HPP
#define DBUCT_CUMULATIVE_GROUNDED_WEIGHT_HPP

#include <deque>
#include <list>
#include <stack>
#include "value_objects/cumulative_grounded_weight_action.hpp"
#include "debug_assert.hpp"

struct dbuct_cumulative_grounded_weight {
    dbuct_cumulative_grounded_weight();
    void accumulate(double w);
    double get() const;

    void push_frame();
    void pop_frame();

private:
    struct frame {
        std::list<cumulative_grounded_weight_action> actions_;
    };

    void log(cumulative_grounded_weight_action action);
    void undo_action(const cumulative_grounded_weight_action& action);

    double value_;
    std::stack<frame> frame_stack_;
};

#endif
