#include "../../../hpp/domain/events/deciding_event.hpp"

std::ostream& operator<<(std::ostream& os, const deciding_event&) {
    return os << "deciding_event{}";
}
