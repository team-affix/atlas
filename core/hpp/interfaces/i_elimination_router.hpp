#ifndef I_ELIMINATION_ROUTER_HPP
#define I_ELIMINATION_ROUTER_HPP

#include "../value_objects/lineage.hpp"
#include "../value_objects/elimination_result.hpp"

struct i_elimination_router {
    virtual ~i_elimination_router() = default;
    virtual elimination_result route(const resolution_lineage*) = 0;
};

#endif
