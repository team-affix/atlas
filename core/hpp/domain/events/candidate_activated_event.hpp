#ifndef CANDIDATE_ACTIVATED_EVENT_HPP
#define CANDIDATE_ACTIVATED_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct candidate_activated_event {
    const resolution_lineage* rl;
};

std::ostream& operator<<(std::ostream&, const candidate_activated_event&);

#endif
