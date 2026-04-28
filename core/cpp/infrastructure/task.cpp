#include "../../hpp/infrastructure/task.hpp"

task::task(uint32_t priority) : scheduler_priority(priority) {
}

uint32_t task::priority() const {
    return scheduler_priority;
}
