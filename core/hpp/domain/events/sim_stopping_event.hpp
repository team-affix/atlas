#ifndef SIM_STOPPING_EVENT_HPP
#define SIM_STOPPING_EVENT_HPP

#include <ostream>

struct sim_stopping_event {
};

std::ostream& operator<<(std::ostream&, const sim_stopping_event&);

#endif
