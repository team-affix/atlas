#ifndef TASK_COMPARE_CPP
#define TASK_COMPARE_CPP

#include "../../hpp/infrastructure/task_compare.hpp"

bool task_compare::operator()(const task* a, const task* b) const {
    return a->priority() > b->priority();
}

#endif
