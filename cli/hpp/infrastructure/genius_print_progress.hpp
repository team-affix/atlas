#ifndef GENIUS_PRINT_PROGRESS_HPP
#define GENIUS_PRINT_PROGRESS_HPP

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <sstream>

template<typename IPrintProgress, typename IRuntime>
struct genius_print_progress {
    genius_print_progress(IPrintProgress& base);
    void set_runtime(IRuntime& rt);
    void on_sim();
    void print();
    void finish_line();
private:
    IPrintProgress&  base_;
    IRuntime*        runtime_;
    double           ema_cgw_;
};

template<typename IPP, typename IRT>
genius_print_progress<IPP, IRT>::genius_print_progress(IPP& base)
    : base_(base)
    , runtime_(nullptr)
    , ema_cgw_(0.0)
{}

template<typename IPP, typename IRT>
void genius_print_progress<IPP, IRT>::set_runtime(IRT& rt) {
    runtime_ = &rt;
    base_.set_runtime(rt);
}

template<typename IPP, typename IRT>
void genius_print_progress<IPP, IRT>::on_sim() {
    base_.on_sim();
    ema_cgw_ = 0.9 * ema_cgw_ + 0.1 * runtime_->cgw();
}

template<typename IPP, typename IRT>
void genius_print_progress<IPP, IRT>::print() {
    base_.print();
    std::ostringstream oss;
    oss << " | cgw " << std::fixed << std::setprecision(2) << ema_cgw_;
    std::cout << oss.str() << std::flush;
}

template<typename IPP, typename IRT>
void genius_print_progress<IPP, IRT>::finish_line() {
    base_.finish_line();
}

#endif
