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
    void set_runtime(IRuntime& rt) { runtime_.emplace(rt); }
    void on_sim();
    void print();
    void finish_line();
    size_t sims_since_last() const { return sims_since_last_; }
private:
    std::string make_line();

    std::optional<std::reference_wrapper<IRuntime>> runtime_;
    size_t total_sims_         = 0;
    size_t sims_since_last_    = 0;
    size_t res_depth_sum_      = 0;
    size_t dec_depth_sum_      = 0;
    double ema_res_depth_      = 0.0;
    double ema_dec_depth_      = 0.0;
    double ema_res_freq_       = 0.0;
    bool   ema_init_           = false;
    std::chrono::steady_clock::time_point last_time_ = std::chrono::steady_clock::now();
    size_t previous_line_width_  = 0;
    bool   progress_line_active_ = false;
};

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

    const auto   now     = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(now - last_time_).count();

    const double cur_res      = n > 0     ? static_cast<double>(res_depth_sum_) / n       : 0.0;
    const double cur_dec      = n > 0     ? static_cast<double>(dec_depth_sum_) / n       : 0.0;
    const double cur_freq     = elapsed > 0.0 ? static_cast<double>(n)             / elapsed : 0.0;
    const double cur_res_freq = elapsed > 0.0 ? static_cast<double>(res_depth_sum_) / elapsed : 0.0;

    if (!ema_init_) {
        ema_res_depth_ = cur_res;
        ema_dec_depth_ = cur_dec;
        ema_res_freq_  = cur_res_freq;
        ema_init_      = true;
    } else {
        ema_res_depth_ = 0.9 * ema_res_depth_ + 0.1 * cur_res;
        ema_dec_depth_ = 0.9 * ema_dec_depth_ + 0.1 * cur_dec;
        ema_res_freq_  = 0.9 * ema_res_freq_  + 0.1 * cur_res_freq;
    }

    sims_since_last_ = 0;
    res_depth_sum_   = 0;
    dec_depth_sum_   = 0;
    last_time_       = now;

    std::ostringstream oss;
    oss << total_sims_ << " sims"
        << " | res " << std::fixed << std::setprecision(1) << ema_res_depth_
        << " | dec " << std::fixed << std::setprecision(1) << ema_dec_depth_
        << " | " << static_cast<size_t>(cur_freq) << " sims/s"
        << " | " << static_cast<size_t>(ema_res_freq_) << " res/s";
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
