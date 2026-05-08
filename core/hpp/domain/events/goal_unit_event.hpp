#ifndef GOAL_UNIT_EVENT_HPP
#define GOAL_UNIT_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct goal_unit_event {
    const goal_lineage* gl;
};

std::ostream& operator<<(std::ostream&, const goal_unit_event&);

#endif
