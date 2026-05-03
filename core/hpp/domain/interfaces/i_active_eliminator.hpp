#ifndef I_ACTIVE_ELIMINATOR_HPP
#define I_ACTIVE_ELIMINATOR_HPP

#include "../value_objects/lineage.hpp"

struct i_active_eliminator {
    virtual ~i_active_eliminator() = default;
    virtual void eliminate(const resolution_lineage*) = 0;
};

#endif
