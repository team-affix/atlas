#ifndef RESOLVING_EVENT_HPP
#define RESOLVING_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct resolving_event {
    const resolution_lineage* rl;
};

std::ostream& operator<<(std::ostream&, const resolving_event&);

#endif
