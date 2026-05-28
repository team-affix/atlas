#ifndef I_ITERATE_ACTIVE_GOALS_HPP
#define I_ITERATE_ACTIVE_GOALS_HPP

#include "value_objects/lineage.hpp"
#include "infrastructure/state_machine.hpp"

struct i_iterate_active_goals {
    virtual ~i_iterate_active_goals() = default;
    virtual state_machine<const goal_lineage*> iterate_active_goals() const = 0;
};

#endif
