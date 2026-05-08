#ifndef GOAL_STORES_CLEARED_EVENT_HPP
#define GOAL_STORES_CLEARED_EVENT_HPP

#include <ostream>

struct goal_stores_cleared_event {
};

std::ostream& operator<<(std::ostream&, const goal_stores_cleared_event&);

#endif
