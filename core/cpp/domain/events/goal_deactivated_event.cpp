#include "../../../hpp/domain/events/goal_deactivated_event.hpp"

std::ostream& operator<<(std::ostream& os, const goal_deactivated_event& e) {
    return os << "goal_deactivated_event{" << e.gl->idx << "}";
}
