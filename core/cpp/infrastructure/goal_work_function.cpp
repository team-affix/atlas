#include "infrastructure/goal_work_function.hpp"

#include <cmath>

goal_work_function::goal_work_function(double k, double j)
    : k_(k)
    , j_(j) {}

double goal_work_function::get(size_t depth) const {
    return 1.0 + std::exp(-k_ * (static_cast<double>(depth) - j_));
}
