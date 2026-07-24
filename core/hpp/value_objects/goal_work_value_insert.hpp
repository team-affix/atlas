#ifndef GOAL_WORK_VALUE_INSERT_HPP
#define GOAL_WORK_VALUE_INSERT_HPP

#include <compare>
#include "value_objects/lineage.hpp"

struct goal_work_value_insert {
    const goal_lineage* gl;
    auto operator<=>(const goal_work_value_insert&) const = default;
};

#endif
