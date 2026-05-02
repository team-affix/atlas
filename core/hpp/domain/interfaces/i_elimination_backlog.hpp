#ifndef I_ELIMINATION_BACKLOG_HPP
#define I_ELIMINATION_BACKLOG_HPP

#include "../value_objects/lineage.hpp"

struct i_elimination_backlog {
    virtual ~i_elimination_backlog() = default;
    virtual void insert(const resolution_lineage*) = 0;
    virtual void goal_activated(const goal_lineage*) = 0;
    virtual void goal_deactivated(const goal_lineage*) = 0;
    virtual void clear() = 0;
};

#endif
