#ifndef DECIDING_EVENT_HPP
#define DECIDING_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct deciding_event {
    const resolution_lineage* rl;
};

std::ostream& operator<<(std::ostream&, const deciding_event&);

#endif
