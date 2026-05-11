#ifndef I_GOAL_RESOLVER_HPP
#define I_GOAL_RESOLVER_HPP

#include "../value_objects/lineage.hpp"

struct i_goal_resolver {
    virtual ~i_goal_resolver() = default;
    virtual void init_resolve(const resolution_lineage*) = 0;
    virtual void resume() = 0;
};

#endif
