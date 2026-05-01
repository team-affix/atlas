#ifndef BACKLOG_ELIMINATION_FLUSHED_EVENT_HPP
#define BACKLOG_ELIMINATION_FLUSHED_EVENT_HPP

#include "../value_objects/lineage.hpp"

struct backlog_elimination_flushed_event {
    const resolution_lineage* rl;
};

#endif
