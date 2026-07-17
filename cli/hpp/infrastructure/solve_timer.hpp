#ifndef SOLVE_TIMER_HPP
#define SOLVE_TIMER_HPP

// Tracks active solving time: accumulates only while resumed. Pause around
// user waits (e.g. Enter between solutions); resume when solving continues.
template<typename INow>
struct solve_timer {
    using duration = typename INow::duration;

    solve_timer(INow& now);

    void pause();
    void resume();
    duration time_since_start() const;

private:
    INow& now_;
    bool running_;
    typename INow::time_point resume_point_;
    duration accumulated_;
};

template<typename INow>
solve_timer<INow>::solve_timer(INow& now)
    : now_(now)
    , running_(false)
    , resume_point_()
    , accumulated_(duration::zero())
{}

template<typename INow>
void solve_timer<INow>::pause() {
    if (!running_)
        return;
    accumulated_ += now_.now() - resume_point_;
    running_ = false;
}

template<typename INow>
void solve_timer<INow>::resume() {
    if (running_)
        return;
    resume_point_ = now_.now();
    running_ = true;
}

template<typename INow>
typename solve_timer<INow>::duration solve_timer<INow>::time_since_start() const {
    if (!running_)
        return accumulated_;
    return accumulated_ + (now_.now() - resume_point_);
}

#endif
