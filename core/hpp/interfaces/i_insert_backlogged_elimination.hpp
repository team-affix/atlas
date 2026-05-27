#ifndef I_INSERT_BACKLOGGED_ELIMINATION_HPP
#define I_INSERT_BACKLOGGED_ELIMINATION_HPP

#include "../value_objects/lineage.hpp"

struct i_insert_backlogged_elimination {
    virtual ~i_insert_backlogged_elimination() = default;
    virtual void insert_backlogged_elimination(const resolution_lineage*) = 0;
};

#endif
