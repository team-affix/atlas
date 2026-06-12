#ifndef PRINT_PROGRESS_HPP
#define PRINT_PROGRESS_HPP

#include "interfaces/i_print_progress.hpp"

struct print_progress : i_print_progress {
    void print(size_t total_sims) override;
    void finish_line() override;

private:
    size_t previous_line_width_ = 0;
    bool progress_line_active_  = false;
};

#endif
