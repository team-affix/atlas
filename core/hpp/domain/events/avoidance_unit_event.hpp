#ifndef AVOIDANCE_UNIT_EVENT_HPP
#define AVOIDANCE_UNIT_EVENT_HPP

#include <cstddef>
#include <ostream>

struct avoidance_unit_event {
    size_t avoidance_id;
};

std::ostream& operator<<(std::ostream&, const avoidance_unit_event&);

#endif
