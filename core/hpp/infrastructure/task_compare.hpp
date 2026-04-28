#ifndef TASK_COMPARE_HPP
#define TASK_COMPARE_HPP

#include "task.hpp"

struct task_compare {
    bool operator()(const task* a, const task* b) const;
};

#endif
