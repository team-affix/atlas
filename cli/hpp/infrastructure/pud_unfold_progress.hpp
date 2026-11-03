#ifndef PUD_UNFOLD_PROGRESS_HPP
#define PUD_UNFOLD_PROGRESS_HPP

#include <chrono>
#include <cstddef>
#include <iostream>
#include <sstream>

// Counts the session's unfolds and prints how many there have been and how long the solve has run.
template<typename IGetTimeSinceStart>
struct pud_unfold_progress {
    pud_unfold_progress(IGetTimeSinceStart& get_time_since_start);
    void on_unfold();
    void print();
private:
    IGetTimeSinceStart& get_time_since_start_;
    size_t total_unfolds_;
    size_t unfolds_since_last_;
};

template<typename IGetTimeSinceStart>
pud_unfold_progress<IGetTimeSinceStart>::pud_unfold_progress(IGetTimeSinceStart& get_time_since_start)
    : get_time_since_start_(get_time_since_start)
    , total_unfolds_(0)
    , unfolds_since_last_(0) {}

template<typename IGetTimeSinceStart>
void pud_unfold_progress<IGetTimeSinceStart>::on_unfold() {
    ++unfolds_since_last_;
}

template<typename IGetTimeSinceStart>
void pud_unfold_progress<IGetTimeSinceStart>::print() {
    if (unfolds_since_last_ == 0)
        return;
    total_unfolds_ += unfolds_since_last_;
    unfolds_since_last_ = 0;
    const double seconds = std::chrono::duration<double>(get_time_since_start_.time_since_start()).count();
    std::ostringstream line;
    line << total_unfolds_ << " unfolds | TSS: " << seconds << "s";
    std::cout << line.str() << '\n' << std::flush;
}

#endif
