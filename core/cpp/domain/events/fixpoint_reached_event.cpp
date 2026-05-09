#include "../../../hpp/domain/events/fixpoint_reached_event.hpp"

std::ostream& operator<<(std::ostream& os, const fixpoint_reached_event&) {
    return os << "fixpoint_reached_event{}";
}
