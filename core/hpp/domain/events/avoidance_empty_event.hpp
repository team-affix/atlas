#ifndef AVOIDANCE_EMPTY_EVENT_HPP
#define AVOIDANCE_EMPTY_EVENT_HPP

#include <cstddef>
#include <ostream>

struct avoidance_empty_event {
    size_t avoidance_id;
};

std::ostream& operator<<(std::ostream&, const avoidance_empty_event&);

#endif
