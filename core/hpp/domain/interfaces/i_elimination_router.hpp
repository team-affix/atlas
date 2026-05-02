#ifndef I_ELIMINATION_ROUTER_HPP
#define I_ELIMINATION_ROUTER_HPP

#include "../value_objects/lineage.hpp"

struct i_elimination_router {
    virtual ~i_elimination_router() = default;
    virtual void route(const resolution_lineage*) = 0;
};

#endif
