#include "../../../hpp/domain/events/sim_started_event.hpp"

std::ostream& operator<<(std::ostream& os, const sim_started_event&) {
    return os << "sim_started_event{}";
}
