#include "../../../hpp/domain/events/sim_cancelled_event.hpp"

std::ostream& operator<<(std::ostream& os, const sim_cancelled_event&) {
    return os << "sim_cancelled_event{}";
}
