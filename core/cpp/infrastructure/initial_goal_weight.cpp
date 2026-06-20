#include "infrastructure/initial_goal_weight.hpp"

initial_goal_weight::initial_goal_weight(double weight) : weight_(weight) {}

double initial_goal_weight::get() const {
    return weight_;
}
