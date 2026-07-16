#ifndef GENIUS_PRINT_PROGRESS_HPP
#define GENIUS_PRINT_PROGRESS_HPP

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "infrastructure/genius_runtime.hpp"

template<typename IPrintProgress>
struct genius_print_progress {
    explicit genius_print_progress(IPrintProgress& base);
    void set_runtime(genius_runtime& rt);
    void on_sim();
    void print();
    void finish_line();
    void note_idle_begin();
    void note_idle_end();
private:
    IPrintProgress&  base_;
    genius_runtime*  runtime_ = nullptr;
    double           ema_cgw_ = 0.0;
};

template<typename IPP>
genius_print_progress<IPP>::genius_print_progress(IPP& base) : base_(base) {}

template<typename IPP>
void genius_print_progress<IPP>::set_runtime(genius_runtime& rt) {
    runtime_ = &rt;
    base_.set_runtime(rt);
}

template<typename IPP>
void genius_print_progress<IPP>::on_sim() {
    base_.on_sim();
    ema_cgw_ = 0.9 * ema_cgw_ + 0.1 * runtime_->cgw();
}

template<typename IPP>
void genius_print_progress<IPP>::print() {
    base_.print();
    std::ostringstream oss;
    oss << " | cgw " << std::fixed << std::setprecision(2) << ema_cgw_;
    std::cout << oss.str() << std::flush;
}

template<typename IPP>
void genius_print_progress<IPP>::finish_line() {
    base_.finish_line();
}

template<typename IPP>
void genius_print_progress<IPP>::note_idle_begin() {
    base_.note_idle_begin();
}

template<typename IPP>
void genius_print_progress<IPP>::note_idle_end() {
    base_.note_idle_end();
}

#endif
