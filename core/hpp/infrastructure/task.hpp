#ifndef TASK_HPP
#define TASK_HPP

#include <cstdint>

struct task {
    virtual ~task() = default;
    task(uint32_t priority);
    uint32_t priority() const;
    virtual void execute() = 0;
#ifndef DEBUG
private:
#endif
    const uint32_t scheduler_priority;
};

#endif
