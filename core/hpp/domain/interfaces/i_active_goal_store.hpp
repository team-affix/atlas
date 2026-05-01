#ifndef I_ACTIVE_GOAL_STORE_HPP
#define I_ACTIVE_GOAL_STORE_HPP

#include "../value_objects/lineage.hpp"

struct i_active_goal_store {
    virtual ~i_active_goal_store() = default;
    virtual void insert(const goal_lineage*) = 0;
    virtual void erase(const goal_lineage*) = 0;
    virtual void clear() = 0;
    virtual bool contains(const goal_lineage*) = 0;
};

#endif
