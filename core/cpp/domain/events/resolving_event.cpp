#include "../../../hpp/domain/events/resolving_event.hpp"

std::ostream& operator<<(std::ostream& os, const resolving_event& e) {
    return os << "resolving_event{" << (e.rl ? std::to_string(e.rl->idx) : "null") << "}";
}
