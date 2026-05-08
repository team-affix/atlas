#include "../../../hpp/domain/events/solved_event.hpp"

std::ostream& operator<<(std::ostream& os, const solved_event&) {
    return os << "solved_event{}";
}
