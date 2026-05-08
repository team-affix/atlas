#ifndef SIM_CANCELLATION_RESET_EVENT_HPP
#define SIM_CANCELLATION_RESET_EVENT_HPP

#include <ostream>

struct sim_cancellation_reset_event {
};

std::ostream& operator<<(std::ostream&, const sim_cancellation_reset_event&);

#endif
