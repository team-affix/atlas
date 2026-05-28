#ifndef I_RESOLVER_HPP
#define I_RESOLVER_HPP

#include "value_objects/lineage.hpp"

struct i_resolver {
    virtual ~i_resolver() = default;
    virtual bool resolve(const resolution_lineage*) = 0;
};

#endif
