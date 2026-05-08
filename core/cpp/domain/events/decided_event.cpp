#include "../../../hpp/domain/events/decided_event.hpp"

std::ostream& operator<<(std::ostream& os, const decided_event& e) {
    return os << "decided_event{" << e.rl->idx << "}";
}
