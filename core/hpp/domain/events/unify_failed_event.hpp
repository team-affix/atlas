#ifndef UNIFY_FAILED_EVENT_HPP
#define UNIFY_FAILED_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct unify_failed_event {
    const resolution_lineage* rl;
};

std::ostream& operator<<(std::ostream&, const unify_failed_event&);

#endif
