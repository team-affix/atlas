#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <queue>
#include "task.hpp"

struct scheduler {
    void schedule(task*);
    void tick();
#ifndef DEBUG
private:
#endif
    std::priority_queue<task*> tasks;
};

#endif
