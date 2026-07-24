#ifndef DBUCT_REMAINING_WORK_HPP
#define DBUCT_REMAINING_WORK_HPP

#include <deque>
#include <list>
#include <stack>
#include "value_objects/remaining_work_action.hpp"
#include "debug_assert.hpp"

struct dbuct_remaining_work {
    dbuct_remaining_work();
    void add(double w);
    void subtract(double w);
    double get() const;

    void push_frame();
    void pop_frame();

private:
    struct frame {
        std::list<remaining_work_action> actions_;
    };

    void log(remaining_work_action action);
    void undo_action(const remaining_work_action& action);

    double value_;
    std::stack<frame> frame_stack_;
};

#endif
