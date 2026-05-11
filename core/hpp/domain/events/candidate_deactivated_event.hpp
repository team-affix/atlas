#ifndef CANDIDATE_DEACTIVATED_EVENT_HPP
#define CANDIDATE_DEACTIVATED_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct candidate_deactivated_event {
    const resolution_lineage* rl;
};

std::ostream& operator<<(std::ostream&, const candidate_deactivated_event&);

#endif
