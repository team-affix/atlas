#ifndef CANDIDATE_ELIMINATED_EVENT_HPP
#define CANDIDATE_ELIMINATED_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct candidate_eliminated_event {
    const resolution_lineage* rl;
};

std::ostream& operator<<(std::ostream&, const candidate_eliminated_event&);

#endif
