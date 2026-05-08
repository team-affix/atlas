#include "../../../hpp/domain/events/sim_cancellation_reset_event.hpp"

std::ostream& operator<<(std::ostream& os, const sim_cancellation_reset_event&) {
    return os << "sim_cancellation_reset_event{}";
}
