#ifndef SOLUTION_DETECTOR_HPP
#define SOLUTION_DETECTOR_HPP

#include "../interfaces/i_solution_detector.hpp"
#include "../interfaces/i_active_goals.hpp"

struct solution_detector : i_solution_detector {
    solution_detector(i_active_goals& ag);
    bool detect() const override;
private:
    i_active_goals& ag;
};

#endif
