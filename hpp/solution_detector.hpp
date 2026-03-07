#ifndef SOLUTION_DETECTOR_HPP
#define SOLUTION_DETECTOR_HPP

#include "a01_defs.hpp"

struct solution_detector {
    solution_detector(
        const a01_goal_store&
    );
    bool operator()();
#ifndef DEBUG
private:
#endif
    const a01_goal_store& gs;
};

#endif
