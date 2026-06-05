#ifndef I_ITERATE_CHILD_GOALS_HPP
#define I_ITERATE_CHILD_GOALS_HPP

#include "value_objects/lineage.hpp"
#include "infrastructure/coroutine.hpp"

struct i_iterate_child_goals {
    virtual ~i_iterate_child_goals() = default;
    virtual coroutine<const goal_lineage*, void> iterate_child_goals(const goal_lineage*) const = 0;
};

#endif
