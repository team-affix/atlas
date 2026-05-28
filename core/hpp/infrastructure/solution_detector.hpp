#ifndef SOLUTION_DETECTOR_HPP
#include "infrastructure/locator.hpp"

#define SOLUTION_DETECTOR_HPP

#include "interfaces/i_solution_detector.hpp"
#include "interfaces/i_check_active_goals_empty.hpp"

struct solution_detector : i_solution_detector {
    solution_detector(locator& loc);
    bool detect() const override;
private:
    i_check_active_goals_empty& check_active_goals_empty;
};

#endif
