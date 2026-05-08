#include "../../../hpp/domain/events/conflicted_event.hpp"

std::ostream& operator<<(std::ostream& os, const conflicted_event&) {
    return os << "conflicted_event{}";
}
