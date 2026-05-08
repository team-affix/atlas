#include "../../../hpp/domain/events/goal_unit_event.hpp"

std::ostream& operator<<(std::ostream& os, const goal_unit_event& e) {
    return os << "goal_unit_event{" << e.gl->idx << "}";
}
