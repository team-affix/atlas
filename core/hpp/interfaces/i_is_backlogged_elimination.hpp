#ifndef I_IS_BACKLOGGED_ELIMINATION_HPP
#define I_IS_BACKLOGGED_ELIMINATION_HPP

#include "../value_objects/lineage.hpp"

struct i_is_backlogged_elimination {
    virtual ~i_is_backlogged_elimination() = default;
    virtual bool is_backlogged_elimination(const resolution_lineage*) const = 0;
};

#endif
