#ifndef SIM_STOPPED_EVENT_HPP
#define SIM_STOPPED_EVENT_HPP

#include <ostream>

struct sim_stopped_event {
};

std::ostream& operator<<(std::ostream&, const sim_stopped_event&);

#endif
