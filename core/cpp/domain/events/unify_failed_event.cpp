#include "../../../hpp/domain/events/unify_failed_event.hpp"

std::ostream& operator<<(std::ostream& os, const unify_failed_event&) {
    return os << "unify_failed_event{}";
}
