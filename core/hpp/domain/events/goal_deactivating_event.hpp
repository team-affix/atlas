#ifndef GOAL_DEACTIVATING_EVENT_HPP
#define GOAL_DEACTIVATING_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct goal_deactivating_event {
    const goal_lineage* gl;
};

std::ostream& operator<<(std::ostream&, const goal_deactivating_event&);

#endif
