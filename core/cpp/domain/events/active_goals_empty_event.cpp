#include "../../../hpp/domain/events/active_goals_empty_event.hpp"

std::ostream& operator<<(std::ostream& os, const active_goals_empty_event&) {
    return os << "active_goals_empty_event{}";
}
