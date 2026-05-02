#ifndef BACKLOGGED_ELIMINATION_FREED_EVENT_HPP
#define BACKLOGGED_ELIMINATION_FREED_EVENT_HPP

#include "../value_objects/lineage.hpp"

struct backlogged_elimination_freed_event {
    const goal_lineage* gl;
    size_t idx;
};

#endif
