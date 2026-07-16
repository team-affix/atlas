#ifndef PRINT_PROGRESS_HPP
#define PRINT_PROGRESS_HPP

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <functional>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <unistd.h>

template<typename IRuntime>
struct print_progress {
    print_progress() = default;
    void set_runtime(IRuntime& rt);
    void on_sim();
    void print();
    void finish_line();
    void note_idle_begin();
    void note_idle_end();
    size_t sims_since_last() const { return sims_since_last_; }
private:
    std::string make_line();

    std::optional<std::reference_wrapper<IRuntime>> runtime_;
    size_t total_sims_      = 0;
    size_t sims_since_last_ = 0;
    size_t res_depth_sum_   = 0;
    size_t dec_depth_sum_   = 0;

    std::chrono::steady_clock::time_point last_time_ =
        std::chrono::steady_clock::now();
    std::optional<std::chrono::steady_clock::time_point> idle_start_;
    double idle_owed_seconds_ = 0.0;

    size_t previous_line_width_  = 0;
    bool   progress_line_active_ = false;
};

template<typename IRuntime>
void print_progress<IRuntime>::set_runtime(IRuntime& rt) {
    runtime_.emplace(rt);
    last_time_          = std::chrono::steady_clock::now();
    idle_start_.reset();
    idle_owed_seconds_  = 0.0;
}

template<typename IRuntime>
void print_progress<IRuntime>::note_idle_begin() {
    if (idle_start_.has_value())
        return;
    idle_start_ = std::chrono::steady_clock::now();
}

template<typename IRuntime>
void print_progress<IRuntime>::note_idle_end() {
    if (!idle_start_.has_value())
        return;
    const auto now = std::chrono::steady_clock::now();
    idle_owed_seconds_ += std::chrono::duration<double>(now - *idle_start_).count();
    idle_start_.reset();
}

template<typename IRuntime>
void print_progress<IRuntime>::on_sim() {
    ++sims_since_last_;
    res_depth_sum_ += runtime_->get().resolution_depth();
    dec_depth_sum_ += runtime_->get().decision_depth();
}

template<typename IRuntime>
std::string print_progress<IRuntime>::make_line() {
    const size_t n = sims_since_last_;
    total_sims_ += n;

    const auto   now             = std::chrono::steady_clock::now();
    const double wall_elapsed    = std::chrono::duration<double>(now - last_time_).count();
    const double elapsed         = wall_elapsed > idle_owed_seconds_
        ? wall_elapsed - idle_owed_seconds_
        : 0.0;
    idle_owed_seconds_ = 0.0;

    const double cur_res      = n > 0 ? static_cast<double>(res_depth_sum_) / static_cast<double>(n) : 0.0;
    const double cur_dec      = n > 0 ? static_cast<double>(dec_depth_sum_) / static_cast<double>(n) : 0.0;
    const double cur_freq     = elapsed > 0.0 ? static_cast<double>(n) / elapsed : 0.0;
    const double cur_res_freq = elapsed > 0.0 ? static_cast<double>(res_depth_sum_) / elapsed : 0.0;

    sims_since_last_ = 0;
    res_depth_sum_   = 0;
    dec_depth_sum_   = 0;
    last_time_       = now;

    std::ostringstream oss;
    oss << total_sims_ << " sims"
        << " | res " << std::fixed << std::setprecision(1) << cur_res
        << " | dec " << std::fixed << std::setprecision(1) << cur_dec
        << " | " << static_cast<size_t>(cur_freq) << " sims/s"
        << " | " << static_cast<size_t>(cur_res_freq) << " res/s";
    return oss.str();
}

template<typename IRuntime>
void print_progress<IRuntime>::print() {
    if (sims_since_last_ == 0) return;
    const std::string text = make_line();
    if (isatty(fileno(stdout))) {
        const std::string padding(
            text.size() < previous_line_width_
                ? previous_line_width_ - text.size()
                : 0,
            ' ');
        std::cout << '\r' << text << padding << std::flush;
        previous_line_width_ = text.size();
    } else {
        std::cout << '\n' << text << std::flush;
    }
    progress_line_active_ = true;
}

template<typename IRuntime>
void print_progress<IRuntime>::finish_line() {
    if (progress_line_active_) {
        std::cout << '\n' << std::flush;
        progress_line_active_ = false;
        previous_line_width_  = 0;
    }
}

#endif
