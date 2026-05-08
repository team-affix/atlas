#ifndef REFUTED_EVENT_HPP
#define REFUTED_EVENT_HPP

#include <ostream>

struct refuted_event {
};

std::ostream& operator<<(std::ostream&, const refuted_event&);

#endif
