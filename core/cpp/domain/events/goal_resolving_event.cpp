#include "../../../hpp/domain/events/goal_resolving_event.hpp"

std::ostream& operator<<(std::ostream& os, const goal_resolving_event& e) {
    return os << "goal_resolving_event{" << e.rl->idx << "}";
}
