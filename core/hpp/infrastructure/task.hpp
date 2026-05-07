#ifndef TASK_HPP
#define TASK_HPP

#include <cstdint>

struct task {
    virtual ~task() = default;
    task(uint32_t);
    uint32_t priority() const;
    virtual void execute() = 0;
private:
    const uint32_t scheduler_priority;
};

#endif
