#ifndef GOAL_WORK_VALUE_ERASE_HPP
#define GOAL_WORK_VALUE_ERASE_HPP

#include <compare>
#include "value_objects/lineage.hpp"

struct goal_work_value_erase {
    const goal_lineage* gl;
    double previous_work;
    auto operator<=>(const goal_work_value_erase&) const = default;
};

#endif
