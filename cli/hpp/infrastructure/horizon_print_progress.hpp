#ifndef HORIZON_PRINT_PROGRESS_HPP
#define HORIZON_PRINT_PROGRESS_HPP

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "infrastructure/horizon_runtime.hpp"

template<typename IPrintProgress>
struct horizon_print_progress {
    explicit horizon_print_progress(IPrintProgress& base);
    void set_runtime(horizon_runtime& rt);
    void on_sim();
    void print();
    void finish_line();
    void note_idle_begin();
    void note_idle_end();
private:
    IPrintProgress&  base_;
    horizon_runtime* runtime_ = nullptr;
    double           ema_cgw_ = 0.0;
};

template<typename IPP>
horizon_print_progress<IPP>::horizon_print_progress(IPP& base) : base_(base) {}

template<typename IPP>
void horizon_print_progress<IPP>::set_runtime(horizon_runtime& rt) {
    runtime_ = &rt;
    base_.set_runtime(rt);
}

template<typename IPP>
void horizon_print_progress<IPP>::on_sim() {
    base_.on_sim();
    ema_cgw_ = 0.9 * ema_cgw_ + 0.1 * runtime_->cgw();
}

template<typename IPP>
void horizon_print_progress<IPP>::print() {
    base_.print();
    std::ostringstream oss;
    oss << " | cgw " << std::fixed << std::setprecision(2) << ema_cgw_;
    std::cout << oss.str() << std::flush;
}

template<typename IPP>
void horizon_print_progress<IPP>::finish_line() {
    base_.finish_line();
}

template<typename IPP>
void horizon_print_progress<IPP>::note_idle_begin() {
    base_.note_idle_begin();
}

template<typename IPP>
void horizon_print_progress<IPP>::note_idle_end() {
    base_.note_idle_end();
}

#endif
