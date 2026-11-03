#ifndef PUD_TICK_PROGRESS_HPP
#define PUD_TICK_PROGRESS_HPP

#include <chrono>
#include <cstddef>
#include <iostream>
#include <sstream>

template<typename IGetTimeSinceStart, typename IGetUnfoldCount>
struct pud_tick_progress {
    pud_tick_progress(IGetTimeSinceStart& get_time_since_start, IGetUnfoldCount& get_unfold_count);
    void on_tick();
    void print();
private:
    IGetTimeSinceStart& get_time_since_start_;
    IGetUnfoldCount& get_unfold_count_;
    size_t total_ticks_;
    size_t ticks_since_last_;
};

template<typename IGetTimeSinceStart, typename IGetUnfoldCount>
pud_tick_progress<IGetTimeSinceStart, IGetUnfoldCount>::pud_tick_progress(
    IGetTimeSinceStart& get_time_since_start, IGetUnfoldCount& get_unfold_count)
    : get_time_since_start_(get_time_since_start)
    , get_unfold_count_(get_unfold_count)
    , total_ticks_(0)
    , ticks_since_last_(0) {}

template<typename IGetTimeSinceStart, typename IGetUnfoldCount>
void pud_tick_progress<IGetTimeSinceStart, IGetUnfoldCount>::on_tick() {
    ++ticks_since_last_;
}

template<typename IGetTimeSinceStart, typename IGetUnfoldCount>
void pud_tick_progress<IGetTimeSinceStart, IGetUnfoldCount>::print() {
    if (ticks_since_last_ == 0)
        return;
    total_ticks_ += ticks_since_last_;
    ticks_since_last_ = 0;
    const double seconds = std::chrono::duration<double>(get_time_since_start_.time_since_start()).count();
    std::ostringstream line;
    line << total_ticks_ << " ticks | " << get_unfold_count_.unfold_count() << " unfolds | TSS: " << seconds << "s";
    std::cout << line.str() << '\n' << std::flush;
}


#endif
