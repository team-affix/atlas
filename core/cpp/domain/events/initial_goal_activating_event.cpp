#include "../../../hpp/domain/events/initial_goal_activating_event.hpp"

std::ostream& operator<<(std::ostream& os, const initial_goal_activating_event& e) {
    return os << "initial_goal_activating_event{" << e.gl->idx << "}";
}
