#include "../../../hpp/domain/events/sim_stopped_event.hpp"

std::ostream& operator<<(std::ostream& os, const sim_stopped_event&) {
    return os << "sim_stopped_event{}";
}
