#include "../../../hpp/domain/events/resolved_event.hpp"

std::ostream& operator<<(std::ostream& os, const resolved_event& e) {
    return os << "resolved_event{" << (e.rl ? std::to_string(e.rl->idx) : "null") << "}";
}
