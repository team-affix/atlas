#include "../../../hpp/domain/events/sim_stopping_event.hpp"

std::ostream& operator<<(std::ostream& os, const sim_stopping_event&) {
    return os << "sim_stopping_event{}";
}
