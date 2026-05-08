#ifndef DECIDED_EVENT_HPP
#define DECIDED_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct decided_event {
    const resolution_lineage* rl;
};

std::ostream& operator<<(std::ostream&, const decided_event&);

#endif
