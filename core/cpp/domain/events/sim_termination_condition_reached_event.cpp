#include "../../../hpp/domain/events/sim_termination_condition_reached_event.hpp"

std::ostream& operator<<(std::ostream& os, const sim_termination_condition_reached_event&) {
    return os << "sim_termination_condition_reached_event{}";
}
