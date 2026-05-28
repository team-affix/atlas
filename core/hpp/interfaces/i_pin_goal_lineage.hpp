#ifndef I_PIN_GOAL_LINEAGE_HPP
#define I_PIN_GOAL_LINEAGE_HPP

#include "../value_objects/lineage.hpp"

struct i_pin_goal_lineage {
    virtual ~i_pin_goal_lineage() = default;
    virtual void pin(const goal_lineage*) = 0;
};

#endif

