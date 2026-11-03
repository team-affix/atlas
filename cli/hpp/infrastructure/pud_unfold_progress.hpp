#ifndef PUD_UNFOLD_PROGRESS_HPP
#define PUD_UNFOLD_PROGRESS_HPP

#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

// Counts the session's unfolds and prints a progress line: total unfolds, unfolds per second since the
// previous line, and how long the solve has run. On a terminal each line overwrites the previous one in
// place, like print_progress for the sim solvers; otherwise each goes on its own line. finish_line()
// ends an in-place line so other output starts on a fresh one.
template<typename IGetTimeSinceStart>
struct pud_unfold_progress {
    pud_unfold_progress(IGetTimeSinceStart& get_time_since_start, bool overwrite_line);
    void on_unfold();
    void print();
    void finish_line();
private:
    IGetTimeSinceStart& get_time_since_start_;
    bool overwrite_line_;
    size_t total_unfolds_;
    size_t unfolds_since_last_;
    double last_seconds_;
    size_t previous_line_width_;
    bool line_active_;
};

template<typename IGetTimeSinceStart>
pud_unfold_progress<IGetTimeSinceStart>::pud_unfold_progress(
    IGetTimeSinceStart& get_time_since_start,
    bool overwrite_line)
    : get_time_since_start_(get_time_since_start)
    , overwrite_line_(overwrite_line)
    , total_unfolds_(0)
    , unfolds_since_last_(0)
    , last_seconds_(0.0)
    , previous_line_width_(0)
    , line_active_(false) {}

template<typename IGetTimeSinceStart>
void pud_unfold_progress<IGetTimeSinceStart>::on_unfold() {
    ++unfolds_since_last_;
}

template<typename IGetTimeSinceStart>
void pud_unfold_progress<IGetTimeSinceStart>::print() {
    if (unfolds_since_last_ == 0)
        return;
    const double seconds = std::chrono::duration<double>(get_time_since_start_.time_since_start()).count();
    const double elapsed = seconds - last_seconds_;
    const double unfolds_per_second = elapsed > 0.0 ? static_cast<double>(unfolds_since_last_) / elapsed : 0.0;
    total_unfolds_ += unfolds_since_last_;
    unfolds_since_last_ = 0;
    last_seconds_ = seconds;
    std::ostringstream line;
    line << total_unfolds_ << " unfolds"
         << " | " << static_cast<size_t>(unfolds_per_second) << " unfolds/s"
         << " | TSS: " << std::fixed << std::setprecision(1) << seconds << "s";
    const std::string text = line.str();
    if (!overwrite_line_) {
        std::cout << text << '\n' << std::flush;
        return;
    }
    const size_t padding_width = text.size() < previous_line_width_ ? previous_line_width_ - text.size() : 0;
    std::cout << '\r' << text << std::string(padding_width, ' ') << "\033[K" << std::flush;
    previous_line_width_ = text.size();
    line_active_ = true;
}

template<typename IGetTimeSinceStart>
void pud_unfold_progress<IGetTimeSinceStart>::finish_line() {
    if (!line_active_)
        return;
    std::cout << '\n' << std::flush;
    line_active_ = false;
    previous_line_width_ = 0;
}

#endif
