#ifndef I_CONSTRAIN_ELIMINATION_BACKLOG_HPP
#define I_CONSTRAIN_ELIMINATION_BACKLOG_HPP

#include "value_objects/lineage.hpp"

struct i_constrain_elimination_backlog {
    virtual ~i_constrain_elimination_backlog() = default;
    virtual void constrain_elimination_backlog(const resolution_lineage*) = 0;
};

#endif
