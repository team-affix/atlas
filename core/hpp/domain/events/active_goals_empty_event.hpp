#ifndef ACTIVE_GOALS_EMPTY_EVENT_HPP
#define ACTIVE_GOALS_EMPTY_EVENT_HPP

#include <ostream>

struct active_goals_empty_event {
};

std::ostream& operator<<(std::ostream&, const active_goals_empty_event&);

#endif
