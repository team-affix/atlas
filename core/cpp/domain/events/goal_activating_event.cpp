#include "../../../hpp/domain/events/goal_activating_event.hpp"

std::ostream& operator<<(std::ostream& os, const goal_activating_event& e) {
    return os << "goal_activating_event{" << e.gl->idx << "}";
}
