#ifndef UNIFY_YIELDED_EVENT_HPP
#define UNIFY_YIELDED_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct unify_yielded_event {
    const resolution_lineage* rl;
};

std::ostream& operator<<(std::ostream&, const unify_yielded_event&);

#endif
