#ifndef GOAL_RESOLVED_EVENT_HPP
#define GOAL_RESOLVED_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct goal_resolved_event {
    const resolution_lineage* rl;
};

std::ostream& operator<<(std::ostream&, const goal_resolved_event&);

#endif
