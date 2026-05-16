#ifndef I_CONFLICT_DETECTOR_HPP
#define I_CONFLICT_DETECTOR_HPP

#include "../value_objects/goal.hpp"

struct i_conflict_detector {
    virtual ~i_conflict_detector() = default;
    virtual bool detect(const goal&) = 0;
};

#endif
