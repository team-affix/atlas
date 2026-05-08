#include "../../../hpp/domain/events/sim_starting_event.hpp"

std::ostream& operator<<(std::ostream& os, const sim_starting_event&) {
    return os << "sim_starting_event{}";
}
