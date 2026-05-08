#include "../../../hpp/domain/events/goal_resolved_event.hpp"

std::ostream& operator<<(std::ostream& os, const goal_resolved_event& e) {
    return os << "goal_resolved_event{" << e.rl->idx << "}";
}
