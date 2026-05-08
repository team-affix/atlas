#ifndef INITIAL_GOAL_ACTIVATING_EVENT_HPP
#define INITIAL_GOAL_ACTIVATING_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct initial_goal_activating_event {
    const goal_lineage* gl;
};

std::ostream& operator<<(std::ostream&, const initial_goal_activating_event&);

#endif
