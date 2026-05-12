#ifndef CANDIDATE_ELIMINATING_EVENT_HPP
#define CANDIDATE_ELIMINATING_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct candidate_eliminating_event {
    const resolution_lineage* rl;
};

std::ostream& operator<<(std::ostream&, const candidate_eliminating_event&);

#endif
