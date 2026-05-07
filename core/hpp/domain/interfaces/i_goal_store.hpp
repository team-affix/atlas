#ifndef I_GOAL_STORE_HPP
#define I_GOAL_STORE_HPP

#include "../value_objects/lineage.hpp"

template<typename T>
struct i_goal_store {
    virtual ~i_goal_store() = default;
    virtual void insert(const goal_lineage*, T) = 0;
    virtual void erase(const goal_lineage*) = 0;
    virtual void clear() = 0;
    virtual T at(const goal_lineage*) const = 0;
};

#endif
