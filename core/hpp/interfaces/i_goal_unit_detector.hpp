#ifndef I_GOAL_UNIT_DETECTOR_HPP
#define I_GOAL_UNIT_DETECTOR_HPP

#include "../value_objects/lineage.hpp"

struct i_goal_unit_detector {
    virtual void candidates_changed(const goal_lineage*) = 0;
};

#endif
