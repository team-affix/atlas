#ifndef I_GOAL_CANDIDATES_EMPTY_DETECTOR_HPP
#define I_GOAL_CANDIDATES_EMPTY_DETECTOR_HPP

#include "../value_objects/lineage.hpp"

struct i_goal_candidates_empty_detector {
    virtual void candidates_changed(const goal_lineage*) = 0;
};

#endif
