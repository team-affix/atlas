#ifndef I_MAKE_RESOLUTION_LINEAGE_HPP
#define I_MAKE_RESOLUTION_LINEAGE_HPP

#include "../value_objects/lineage.hpp"

struct i_make_resolution_lineage {
    virtual ~i_make_resolution_lineage() = default;
    virtual const resolution_lineage* make_resolution_lineage(
        const goal_lineage* parent, rule_id idx) = 0;
};

#endif

