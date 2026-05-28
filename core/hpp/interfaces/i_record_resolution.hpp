#ifndef I_RECORD_RESOLUTION_HPP
#define I_RECORD_RESOLUTION_HPP

#include "value_objects/lineage.hpp"

struct i_record_resolution {
    virtual ~i_record_resolution() = default;
    virtual void record_resolution(const resolution_lineage*) = 0;
};

#endif

