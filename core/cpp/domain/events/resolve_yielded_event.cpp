#include "../../../hpp/domain/events/resolve_yielded_event.hpp"

std::ostream& operator<<(std::ostream& os, const resolve_yielded_event&) {
    return os << "resolve_yielded_event{}";
}
