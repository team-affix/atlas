#include "../../../hpp/domain/events/goal_candidates_empty_event.hpp"

std::ostream& operator<<(std::ostream& os, const goal_candidates_empty_event& e) {
    return os << "goal_candidates_empty_event{" << e.gl->idx << "}";
}
