#ifndef SIM_CANCELLED_EVENT_HPP
#define SIM_CANCELLED_EVENT_HPP

#include <ostream>

struct sim_cancelled_event {
};

std::ostream& operator<<(std::ostream&, const sim_cancelled_event&);

#endif
