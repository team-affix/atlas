#ifndef UNIFY_FUNCTOR_COMPLETED_EVENT_HPP
#define UNIFY_FUNCTOR_COMPLETED_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct unify_functor_completed_event {
    const resolution_lineage* rl;
};

std::ostream& operator<<(std::ostream&, const unify_functor_completed_event&);

#endif
