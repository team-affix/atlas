#include "infrastructure/print_progress.hpp"
#include <cstdio>
#include <iostream>
#include <string>
#include <unistd.h>

void print_progress::print(size_t total_sims) {
    const std::string line = std::to_string(total_sims) + " sims";
    if (isatty(fileno(stdout)) != 0) {
        std::cout << '\r' << line;
        if (line.size() < previous_line_width_)
            std::cout << std::string(previous_line_width_ - line.size(), ' ');
        previous_line_width_   = line.size();
        progress_line_active_  = true;
        std::cout << std::flush;
        return;
    }
    std::cout << '\n' << line << '\n';
}

void print_progress::finish_line() {
    if (!progress_line_active_)
        return;
    std::cout << '\n';
    progress_line_active_ = false;
}
