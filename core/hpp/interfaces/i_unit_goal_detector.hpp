#ifndef I_UNIT_GOAL_DETECTOR_HPP
#define I_UNIT_GOAL_DETECTOR_HPP

#include "../value_objects/lineage.hpp"

struct i_unit_goal_detector {
    virtual ~i_unit_goal_detector() = default;
    virtual bool detect(const goal_lineage*) const = 0;
};

#endif
