#ifndef CANDIDATE_ACTIVATING_EVENT_HPP
#define CANDIDATE_ACTIVATING_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct candidate_activating_event {
    const resolution_lineage* rl;
};

std::ostream& operator<<(std::ostream&, const candidate_activating_event&);

#endif
