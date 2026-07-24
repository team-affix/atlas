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

template<typename IRuntime, typename IGetTimeSinceStart>
struct print_progress {
    print_progress(IGetTimeSinceStart& get_time_since_start);
    void set_runtime(IRuntime& rt);
    void on_sim();
    void print();
    void finish_line();
    size_t sims_since_last() const { return sims_since_last_; }
private:
    std::string make_line();

    IGetTimeSinceStart& get_time_since_start_;
    std::optional<std::reference_wrapper<IRuntime>> runtime_;
    size_t total_sims_;
    size_t sims_since_last_;
    size_t res_depth_sum_;
    size_t dec_depth_sum_;
    typename IGetTimeSinceStart::duration last_tss_;
    size_t previous_line_width_;
    bool progress_line_active_;
};

template<typename IRuntime, typename IGTS>
print_progress<IRuntime, IGTS>::print_progress(IGTS& get_time_since_start)
    : get_time_since_start_(get_time_since_start)
    , runtime_{}
    , total_sims_(0)
    , sims_since_last_(0)
    , res_depth_sum_(0)
    , dec_depth_sum_(0)
    , last_tss_(IGTS::duration::zero())
    , previous_line_width_(0)
    , progress_line_active_(false)
{}

template<typename IRuntime, typename IGTS>
void print_progress<IRuntime, IGTS>::set_runtime(IRuntime& rt) {
    runtime_.emplace(rt);
    last_tss_ = get_time_since_start_.time_since_start();
}

template<typename IRuntime, typename IGTS>
void print_progress<IRuntime, IGTS>::on_sim() {
    ++sims_since_last_;
    res_depth_sum_ += runtime_->get().resolution_depth();
    dec_depth_sum_ += runtime_->get().decision_depth();
}

template<typename IRuntime, typename IGTS>
std::string print_progress<IRuntime, IGTS>::make_line() {
    const size_t n = sims_since_last_;
    total_sims_ += n;

    const auto   now_tss = get_time_since_start_.time_since_start();
    const double elapsed = std::chrono::duration<double>(now_tss - last_tss_).count();
    last_tss_ = now_tss;

    const double cur_res      = n > 0 ? static_cast<double>(res_depth_sum_) / static_cast<double>(n) : 0.0;
    const double cur_dec      = n > 0 ? static_cast<double>(dec_depth_sum_) / static_cast<double>(n) : 0.0;
    const double cur_freq     = elapsed > 0.0 ? static_cast<double>(n) / elapsed : 0.0;
    const double cur_res_freq = elapsed > 0.0 ? static_cast<double>(res_depth_sum_) / elapsed : 0.0;
    const double tss_seconds  = std::chrono::duration<double>(now_tss).count();

    sims_since_last_ = 0;
    res_depth_sum_   = 0;
    dec_depth_sum_   = 0;

    std::ostringstream oss;
    oss << total_sims_ << " sims"
        << " | res " << std::fixed << std::setprecision(1) << cur_res
        << " | dec " << std::fixed << std::setprecision(1) << cur_dec
        << " | " << static_cast<size_t>(cur_freq) << " sims/s"
        << " | " << static_cast<size_t>(cur_res_freq) << " res/s"
        << " | TSS: " << std::fixed << std::setprecision(1) << tss_seconds << "s";
    return oss.str();
}

template<typename IRuntime, typename IGTS>
void print_progress<IRuntime, IGTS>::print() {
    if (sims_since_last_ == 0) return;
    const std::string text = make_line();
    if (isatty(fileno(stdout))) {
        const std::string padding(
            text.size() < previous_line_width_
                ? previous_line_width_ - text.size()
                : 0,
            ' ');
        std::cout << '\r' << text << padding << "\033[K" << std::flush;
        previous_line_width_ = text.size();
    } else {
        std::cout << '\n' << text << std::flush;
    }
    progress_line_active_ = true;
}

template<typename IRuntime, typename IGTS>
void print_progress<IRuntime, IGTS>::finish_line() {
    if (progress_line_active_) {
        std::cout << '\n' << std::flush;
        progress_line_active_ = false;
        previous_line_width_  = 0;
    }
}

#endif
