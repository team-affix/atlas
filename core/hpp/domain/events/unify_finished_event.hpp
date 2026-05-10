#ifndef UNIFY_FINISHED_EVENT_HPP
#define UNIFY_FINISHED_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct unify_finished_event {
    const resolution_lineage* rl;
};

std::ostream& operator<<(std::ostream&, const unify_finished_event&);

#endif
