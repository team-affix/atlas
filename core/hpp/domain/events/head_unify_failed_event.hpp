#ifndef HEAD_UNIFY_FAILED_EVENT_HPP
#define HEAD_UNIFY_FAILED_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct head_unify_failed_event {
    const resolution_lineage* rl;
};

std::ostream& operator<<(std::ostream&, const head_unify_failed_event&);

#endif
