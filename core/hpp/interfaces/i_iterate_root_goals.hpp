#ifndef I_ITERATE_ROOT_GOALS_HPP
#define I_ITERATE_ROOT_GOALS_HPP

#include "value_objects/lineage.hpp"
#include "infrastructure/coroutine.hpp"

struct i_iterate_root_goals {
    virtual ~i_iterate_root_goals() = default;
    virtual coroutine<const goal_lineage*, void> iterate_root_goals() const = 0;
};

#endif
