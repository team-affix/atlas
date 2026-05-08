#ifndef SIM_ENDED_EVENT_HPP
#define SIM_ENDED_EVENT_HPP

#include <ostream>

struct sim_ended_event {
};

std::ostream& operator<<(std::ostream&, const sim_ended_event&);

#endif
