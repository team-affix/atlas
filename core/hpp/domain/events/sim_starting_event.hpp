#ifndef SIM_STARTING_EVENT_HPP
#define SIM_STARTING_EVENT_HPP

#include <ostream>

struct sim_starting_event {
};

std::ostream& operator<<(std::ostream&, const sim_starting_event&);

#endif
