#ifndef CANDIDATE_DEACTIVATING_EVENT_HPP
#define CANDIDATE_DEACTIVATING_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct candidate_deactivating_event {
    const resolution_lineage* rl;
};

std::ostream& operator<<(std::ostream&, const candidate_deactivating_event&);

#endif
