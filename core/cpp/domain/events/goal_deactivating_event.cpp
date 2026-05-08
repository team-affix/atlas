#include "../../../hpp/domain/events/goal_deactivating_event.hpp"

std::ostream& operator<<(std::ostream& os, const goal_deactivating_event& e) {
    return os << "goal_deactivating_event{" << e.gl->idx << "}";
}
