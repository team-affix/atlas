#include "../../../hpp/domain/events/goal_stores_cleared_event.hpp"

std::ostream& operator<<(std::ostream& os, const goal_stores_cleared_event&) {
    return os << "goal_stores_cleared_event{}";
}
