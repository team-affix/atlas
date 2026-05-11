#ifndef RESOLVED_EVENT_HPP
#define RESOLVED_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct resolved_event {
    const resolution_lineage* rl;
};

std::ostream& operator<<(std::ostream&, const resolved_event&);

#endif
