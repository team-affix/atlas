#ifndef UNIFY_RESUMING_EVENT_HPP
#define UNIFY_RESUMING_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct unify_resuming_event {
    const resolution_lineage* rl;
};

std::ostream& operator<<(std::ostream&, const unify_resuming_event&);

#endif
