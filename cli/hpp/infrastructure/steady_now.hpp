#ifndef STEADY_NOW_HPP
#define STEADY_NOW_HPP

#include <chrono>

// Production clock capability for solve_timer: returns steady_clock::now().
struct steady_now {
    using time_point = std::chrono::steady_clock::time_point;
    using duration   = std::chrono::steady_clock::duration;

    time_point now() const;
};

#endif
