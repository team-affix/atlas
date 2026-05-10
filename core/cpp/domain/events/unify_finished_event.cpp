#include "../../../hpp/domain/events/unify_finished_event.hpp"

std::ostream& operator<<(std::ostream& os, const unify_finished_event&) {
    return os << "unify_finished_event{}";
}
