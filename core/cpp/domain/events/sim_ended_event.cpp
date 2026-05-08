#include "../../../hpp/domain/events/sim_ended_event.hpp"

std::ostream& operator<<(std::ostream& os, const sim_ended_event&) {
    return os << "sim_ended_event{}";
}
