#ifndef FIXPOINT_REACHED_EVENT_HPP
#define FIXPOINT_REACHED_EVENT_HPP

#include <ostream>

struct fixpoint_reached_event {
};

std::ostream& operator<<(std::ostream&, const fixpoint_reached_event&);

#endif
