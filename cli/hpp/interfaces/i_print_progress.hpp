#ifndef I_PRINT_PROGRESS_HPP
#define I_PRINT_PROGRESS_HPP

#include <cstddef>

struct i_print_progress {
    virtual ~i_print_progress() = default;
    virtual void print(size_t total_sims) = 0;
    virtual void finish_line() = 0;
};

#endif
