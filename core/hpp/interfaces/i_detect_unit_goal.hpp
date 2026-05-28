#ifndef I_DETECT_UNIT_GOAL_HPP
#define I_DETECT_UNIT_GOAL_HPP

#include "value_objects/lineage.hpp"

struct i_detect_unit_goal {
    virtual ~i_detect_unit_goal() = default;
    virtual bool detect(const goal_lineage*) const = 0;
};

#endif

