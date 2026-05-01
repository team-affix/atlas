#ifndef I_LINEAGE_POOL_HPP
#define I_LINEAGE_POOL_HPP

#include "../value_objects/lineage.hpp"

struct i_lineage_pool {
    virtual ~i_lineage_pool() = default;
    virtual const goal_lineage* goal(const resolution_lineage* parent, size_t index) const = 0;
    virtual const resolution_lineage* resolution(const goal_lineage* parent, size_t index) const = 0;
};

#endif
