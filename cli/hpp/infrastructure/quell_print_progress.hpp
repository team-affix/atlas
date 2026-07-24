#ifndef QUELL_PRINT_PROGRESS_HPP
#define QUELL_PRINT_PROGRESS_HPP

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <unistd.h>

template<typename IPrintProgress, typename IRuntime>
struct quell_print_progress {
    quell_print_progress(IPrintProgress& base);
    void set_runtime(IRuntime& rt);
    void on_sim();
    void print();
    void finish_line();
private:
    IPrintProgress&  base_;
    IRuntime*        runtime_;
    double           lowest_remaining_work_;
};

template<typename IPP, typename IRT>
quell_print_progress<IPP, IRT>::quell_print_progress(IPP& base)
    : base_(base)
    , runtime_(nullptr)
    , lowest_remaining_work_(std::numeric_limits<double>::infinity())
{}

template<typename IPP, typename IRT>
void quell_print_progress<IPP, IRT>::set_runtime(IRT& rt) {
    runtime_ = &rt;
    base_.set_runtime(rt);
}

template<typename IPP, typename IRT>
void quell_print_progress<IPP, IRT>::on_sim() {
    base_.on_sim();
    lowest_remaining_work_ =
        std::min(lowest_remaining_work_, runtime_->remaining_work());
}

template<typename IPP, typename IRT>
void quell_print_progress<IPP, IRT>::print() {
    base_.print();
    std::ostringstream oss;
    oss << " | work " << std::fixed << std::setprecision(2)
        << runtime_->remaining_work()
        << " | min work " << std::fixed << std::setprecision(2)
        << lowest_remaining_work_
        << " | goals " << runtime_->remaining_active_goals();
    std::cout << oss.str();
    if (isatty(fileno(stdout)))
        std::cout << "\033[K";
    std::cout << std::flush;
}

template<typename IPP, typename IRT>
void quell_print_progress<IPP, IRT>::finish_line() {
    base_.finish_line();
}

#endif
