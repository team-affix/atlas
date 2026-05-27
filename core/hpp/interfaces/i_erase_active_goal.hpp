#ifndef I_ERASE_ACTIVE_GOAL_HPP
#define I_ERASE_ACTIVE_GOAL_HPP

#include "../value_objects/lineage.hpp"

struct i_erase_active_goal {
    virtual ~i_erase_active_goal() = default;
    virtual void erase_active_goal(const goal_lineage*) = 0;
};

#endif
