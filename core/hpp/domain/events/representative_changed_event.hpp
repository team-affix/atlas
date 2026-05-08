#ifndef REPRESENTATIVE_CHANGED_EVENT_HPP
#define REPRESENTATIVE_CHANGED_EVENT_HPP

#include <cstdint>
#include <ostream>

struct representative_changed_event {
    uint32_t var_index;
};

std::ostream& operator<<(std::ostream&, const representative_changed_event&);

#endif
