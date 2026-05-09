#include "../../../hpp/domain/events/never_event.hpp"

std::ostream& operator<<(std::ostream& os, const never_event&) {
    return os << "never_event{}";
}
