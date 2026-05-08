#ifndef CANDIDATE_NOT_APPLICABLE_EVENT_HPP
#define CANDIDATE_NOT_APPLICABLE_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct candidate_not_applicable_event {
    const resolution_lineage* rl;
};

std::ostream& operator<<(std::ostream&, const candidate_not_applicable_event&);

#endif
