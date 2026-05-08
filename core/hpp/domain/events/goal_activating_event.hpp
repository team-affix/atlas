#ifndef GOAL_ACTIVATING_EVENT_HPP
#define GOAL_ACTIVATING_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct goal_activating_event {
    const goal_lineage* gl;
};

std::ostream& operator<<(std::ostream&, const goal_activating_event&);

#endif
