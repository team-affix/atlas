#ifndef GOAL_RESOLVING_EVENT_HPP
#define GOAL_RESOLVING_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct goal_resolving_event {
    const resolution_lineage* rl;
};

std::ostream& operator<<(std::ostream&, const goal_resolving_event&);

#endif
