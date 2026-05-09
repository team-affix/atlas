#ifndef SIM_TERMINATION_CONDITION_REACHED_EVENT_HPP
#define SIM_TERMINATION_CONDITION_REACHED_EVENT_HPP

#include <ostream>

struct sim_termination_condition_reached_event {
};

std::ostream& operator<<(std::ostream&, const sim_termination_condition_reached_event&);

#endif
