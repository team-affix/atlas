#ifndef I_DEACTIVATED_GOAL_MEMORY_HPP
#define I_DEACTIVATED_GOAL_MEMORY_HPP

#include "../value_objects/lineage.hpp"

struct i_deactivated_goal_memory {
    virtual ~i_deactivated_goal_memory() = default;
    virtual void insert(const goal_lineage*) = 0;
    virtual void clear() = 0;
    virtual bool contains(const goal_lineage*) const = 0;
};

#endif
