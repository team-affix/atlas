#include "../../../hpp/domain/events/avoidance_empty_event.hpp"

std::ostream& operator<<(std::ostream& os, const avoidance_empty_event& e) {
    return os << "avoidance_empty_event{" << e.avoidance_id << "}";
}
