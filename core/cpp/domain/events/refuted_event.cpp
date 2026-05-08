#include "../../../hpp/domain/events/refuted_event.hpp"

std::ostream& operator<<(std::ostream& os, const refuted_event&) {
    return os << "refuted_event{}";
}
