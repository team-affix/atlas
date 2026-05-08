#ifndef GOAL_ACTIVATED_EVENT_HPP
#define GOAL_ACTIVATED_EVENT_HPP

#include <ostream>
#include "../value_objects/lineage.hpp"

struct goal_activated_event {
    const goal_lineage* gl;
};

std::ostream& operator<<(std::ostream&, const goal_activated_event&);

#endif
