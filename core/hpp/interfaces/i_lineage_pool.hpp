#ifndef I_LINEAGE_POOL_HPP
#define I_LINEAGE_POOL_HPP

#include "../value_objects/lineage.hpp"

struct i_lineage_pool {
    virtual ~i_lineage_pool() = default;
    virtual const goal_lineage* goal(const resolution_lineage* parent, size_t index) = 0;
    virtual const resolution_lineage* resolution(const goal_lineage* parent, size_t index) = 0;
    virtual void pin(const goal_lineage*) = 0;
    virtual void pin(const resolution_lineage*) = 0;
    virtual void trim() = 0;
    virtual const goal_lineage* import(const goal_lineage*) = 0;
    virtual const resolution_lineage* import(const resolution_lineage*) = 0;
};

#endif
