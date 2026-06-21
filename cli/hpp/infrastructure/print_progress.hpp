#ifndef PRINT_PROGRESS_HPP
#define PRINT_PROGRESS_HPP

#include <cstddef>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>

struct print_progress {
    void print(size_t total_sims, double avg_res_depth, double avg_dec_depth, double sim_freq, double res_freq);
    void finish_line();
private:
    size_t previous_line_width_ = 0;
    bool progress_line_active_  = false;
};

inline void print_progress::print(
    size_t total_sims, double avg_res_depth, double avg_dec_depth, double sim_freq, double res_freq) {
    std::ostringstream oss;
    oss << total_sims << " sims"
        << " | res " << std::fixed << std::setprecision(1) << avg_res_depth
        << " | dec " << std::fixed << std::setprecision(1) << avg_dec_depth
        << " | " << static_cast<size_t>(sim_freq) << " sims/s"
        << " | " << static_cast<size_t>(res_freq) << " res/s";
    const std::string text = oss.str();
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
