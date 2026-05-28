#ifndef I_MAKE_GOAL_LINEAGE_HPP
#define I_MAKE_GOAL_LINEAGE_HPP

#include "value_objects/lineage.hpp"

struct i_make_goal_lineage {
    virtual ~i_make_goal_lineage() = default;
    virtual const goal_lineage* make_goal_lineage(
        const resolution_lineage* parent, subgoal_id idx) = 0;
};

#endif

