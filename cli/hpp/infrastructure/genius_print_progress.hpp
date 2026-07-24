#ifndef GENIUS_PRINT_PROGRESS_HPP
#define GENIUS_PRINT_PROGRESS_HPP

#include <cstdio>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unistd.h>

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
};

template<typename IPP, typename IRT>
genius_print_progress<IPP, IRT>::genius_print_progress(IPP& base)
    : base_(base)
    , runtime_(nullptr)
{}

template<typename IPP, typename IRT>
void genius_print_progress<IPP, IRT>::set_runtime(IRT& rt) {
    runtime_ = &rt;
    base_.set_runtime(rt);
}

template<typename IPP, typename IRT>
void genius_print_progress<IPP, IRT>::on_sim() {
    base_.on_sim();
}

template<typename IPP, typename IRT>
void genius_print_progress<IPP, IRT>::print() {
    base_.print();
    std::ostringstream oss;
    oss << " | cgw " << std::fixed << std::setprecision(2) << runtime_->cgw();
    std::cout << oss.str();
    if (isatty(fileno(stdout)))
        std::cout << "\033[K";
    std::cout << std::flush;
}

template<typename IPP, typename IRT>
void genius_print_progress<IPP, IRT>::finish_line() {
    base_.finish_line();
}

#endif
