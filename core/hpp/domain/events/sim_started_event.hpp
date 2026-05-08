#ifndef SIM_STARTED_EVENT_HPP
#define SIM_STARTED_EVENT_HPP

#include <ostream>

struct sim_started_event {
};

std::ostream& operator<<(std::ostream&, const sim_started_event&);

#endif
