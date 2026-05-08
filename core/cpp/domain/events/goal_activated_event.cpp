#include "../../../hpp/domain/events/goal_activated_event.hpp"

std::ostream& operator<<(std::ostream& os, const goal_activated_event& e) {
    return os << "goal_activated_event{" << e.gl->idx << "}";
}
