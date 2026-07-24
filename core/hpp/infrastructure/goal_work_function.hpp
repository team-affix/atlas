#ifndef GOAL_WORK_FUNCTION_HPP
#define GOAL_WORK_FUNCTION_HPP

#include <cstddef>

struct goal_work_function {
    goal_work_function(double k, double j);
    double get(size_t depth) const;
private:
    double k_;
    double j_;
};

#endif
