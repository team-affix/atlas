#include "../../../hpp/domain/events/goal_candidates_changed_event.hpp"

std::ostream& operator<<(std::ostream& os, const goal_candidates_changed_event& e) {
    return os << "goal_candidates_changed_event{" << e.gl->idx << "}";
}
