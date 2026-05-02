#ifndef I_GOAL_EXPANDER_HPP
#define I_GOAL_EXPANDER_HPP

#include "../value_objects/lineage.hpp"

struct i_goal_expander {
    virtual ~i_goal_expander() = default;
    virtual void start_expansion(const goal_lineage*) = 0;
    virtual void expand_child(const goal_lineage*) = 0;
};

#endif
