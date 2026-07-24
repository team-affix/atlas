#ifndef QUELL_PRINT_PROGRESS_HPP
#define QUELL_PRINT_PROGRESS_HPP

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <sstream>

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
    double           ema_remaining_work_;
};

template<typename IPP, typename IRT>
quell_print_progress<IPP, IRT>::quell_print_progress(IPP& base)
    : base_(base)
    , runtime_(nullptr)
    , ema_remaining_work_(0.0)
{}

template<typename IPP, typename IRT>
void quell_print_progress<IPP, IRT>::set_runtime(IRT& rt) {
    runtime_ = &rt;
    base_.set_runtime(rt);
}

template<typename IPP, typename IRT>
void quell_print_progress<IPP, IRT>::on_sim() {
    base_.on_sim();
    ema_remaining_work_ = 0.9 * ema_remaining_work_ + 0.1 * runtime_->remaining_work();
}

template<typename IPP, typename IRT>
void quell_print_progress<IPP, IRT>::print() {
    base_.print();
    std::ostringstream oss;
    oss << " | work " << std::fixed << std::setprecision(2) << ema_remaining_work_;
    std::cout << oss.str() << std::flush;
}

template<typename IPP, typename IRT>
void quell_print_progress<IPP, IRT>::finish_line() {
    base_.finish_line();
}

#endif
