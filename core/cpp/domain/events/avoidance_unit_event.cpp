#include "../../../hpp/domain/events/avoidance_unit_event.hpp"

std::ostream& operator<<(std::ostream& os, const avoidance_unit_event& e) {
    return os << "avoidance_unit_event{" << e.avoidance_id << "}";
}
