#ifndef PUD_TICK_PROGRESS_HPP
#define PUD_TICK_PROGRESS_HPP

#include <chrono>
#include <cstddef>
#include <iostream>
#include <sstream>

template<typename IGetTimeSinceStart>
struct pud_tick_progress {
    pud_tick_progress(IGetTimeSinceStart& get_time_since_start);
    void on_tick();
    void print();
    void finish_line();
private:
    IGetTimeSinceStart& get_time_since_start_;
    size_t total_ticks_;
    size_t ticks_since_last_;
};

template<typename IGetTimeSinceStart>
pud_tick_progress<IGetTimeSinceStart>::pud_tick_progress(IGetTimeSinceStart& get_time_since_start)
    : get_time_since_start_(get_time_since_start)
    , total_ticks_(0)
    , ticks_since_last_(0) {}

template<typename IGetTimeSinceStart>
void pud_tick_progress<IGetTimeSinceStart>::on_tick() {
    ++ticks_since_last_;
}

template<typename IGetTimeSinceStart>
void pud_tick_progress<IGetTimeSinceStart>::print() {
    if (ticks_since_last_ == 0)
        return;
    total_ticks_ += ticks_since_last_;
    ticks_since_last_ = 0;
    const double seconds = std::chrono::duration<double>(get_time_since_start_.time_since_start()).count();
    std::ostringstream line;
    line << total_ticks_ << " ticks | TSS: " << seconds << "s";
    std::cout << line.str() << '\n' << std::flush;
}

template<typename IGetTimeSinceStart>
void pud_tick_progress<IGetTimeSinceStart>::finish_line() {}

#endif
