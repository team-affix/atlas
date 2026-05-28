#ifndef I_GET_UNIT_RESOLUTION_HPP
#define I_GET_UNIT_RESOLUTION_HPP

#include "value_objects/lineage.hpp"

struct i_get_unit_resolution {
    virtual ~i_get_unit_resolution() = default;
    virtual const resolution_lineage* get(const goal_lineage*) = 0;
};

#endif
