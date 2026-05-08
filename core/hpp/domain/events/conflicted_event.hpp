#ifndef CONFLICTED_EVENT_HPP
#define CONFLICTED_EVENT_HPP

#include <ostream>

struct conflicted_event {
};

std::ostream& operator<<(std::ostream&, const conflicted_event&);

#endif
