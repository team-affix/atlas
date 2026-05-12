#include "../../../hpp/domain/events/candidate_eliminating_event.hpp"

std::ostream& operator<<(std::ostream& os, const candidate_eliminating_event& e) {
    return os << "candidate_eliminating_event{" << e.rl->idx << "}";
}
