#ifndef I_CONFLICT_DETECTOR_HPP
#define I_CONFLICT_DETECTOR_HPP

#include "../value_objects/lineage.hpp"

struct i_conflict_detector {
    virtual ~i_conflict_detector() = default;
    virtual bool detect(const goal_lineage*) const = 0;
};

#endif
