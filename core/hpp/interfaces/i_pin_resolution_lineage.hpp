#ifndef I_PIN_RESOLUTION_LINEAGE_HPP
#define I_PIN_RESOLUTION_LINEAGE_HPP

#include "../value_objects/lineage.hpp"

struct i_pin_resolution_lineage {
    virtual ~i_pin_resolution_lineage() = default;
    virtual void pin(const resolution_lineage*) = 0;
};

#endif

