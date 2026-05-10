#include "../../../hpp/domain/events/candidate_deactivating_event.hpp"

std::ostream& operator<<(std::ostream& os, const candidate_deactivating_event&) {
    return os << "candidate_deactivating_event{}";
}
