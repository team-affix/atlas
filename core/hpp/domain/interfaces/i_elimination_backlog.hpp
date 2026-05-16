#ifndef I_ELIMINATION_BACKLOG_HPP
#define I_ELIMINATION_BACKLOG_HPP

#include "../value_objects/lineage.hpp"

class i_elimination_backlog {
public:
    virtual ~i_elimination_backlog() = default;
    virtual void push(const resolution_lineage*) = 0;
    virtual bool contains(const resolution_lineage*) = 0;
    virtual void constrain(const resolution_lineage*) = 0;
};

#endif
