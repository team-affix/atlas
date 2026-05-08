#ifndef BACKLOGGED_ELIMINATION_FREED_EVENT_HPP
#define BACKLOGGED_ELIMINATION_FREED_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct backlogged_elimination_freed_event {
    const goal_lineage* gl;
    size_t idx;
};

std::ostream& operator<<(std::ostream&, const backlogged_elimination_freed_event&);

#endif
