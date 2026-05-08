#ifndef GOAL_DEACTIVATED_EVENT_HPP
#define GOAL_DEACTIVATED_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct goal_deactivated_event {
    const goal_lineage* gl;
};

std::ostream& operator<<(std::ostream&, const goal_deactivated_event&);

#endif
