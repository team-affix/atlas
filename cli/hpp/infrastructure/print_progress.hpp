#ifndef PRINT_PROGRESS_HPP
#define PRINT_PROGRESS_HPP

#include <cstddef>
#include <cstdio>
#include <iostream>
#include <string>
#include <unistd.h>

struct print_progress {
    void print(size_t total_sims);
    void finish_line();
private:
    size_t previous_line_width_ = 0;
    bool progress_line_active_  = false;
};

inline void print_progress::print(size_t total_sims) {
    const std::string text = std::to_string(total_sims) + " sims";
    if (isatty(fileno(stdout))) {
        // TTY: overwrite the current line in place.
        const std::string padding(
            text.size() < previous_line_width_
                ? previous_line_width_ - text.size()
                : 0,
            ' ');
        std::cout << '\r' << text << padding << std::flush;
        previous_line_width_  = text.size();
        progress_line_active_ = true;
    } else {
        // Non-TTY: each update is its own newline-terminated line.
        std::cout << '\n' << text << '\n' << std::flush;
    }
}

inline void print_progress::finish_line() {
    if (progress_line_active_) {
        std::cout << '\n' << std::flush;
        progress_line_active_  = false;
        previous_line_width_   = 0;
    }
}

#endif
