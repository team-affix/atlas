#ifndef I_UNIT_GOALS_HPP
#define I_UNIT_GOALS_HPP

#include "../value_objects/lineage.hpp"

struct i_unit_goals {
    virtual ~i_unit_goals() = default;
    virtual void push(const goal_lineage*) = 0;
    virtual const goal_lineage* pop() = 0;
    virtual void clear() = 0;
    virtual bool empty() const = 0;
};

#endif
