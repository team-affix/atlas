#ifndef I_ACTIVATE_SUBGOALS_HPP
#define I_ACTIVATE_SUBGOALS_HPP

#include "value_objects/lineage.hpp"

struct i_activate_subgoals {
    virtual ~i_activate_subgoals() = default;
    virtual bool activate_subgoals(const resolution_lineage*) = 0;
};

#endif
