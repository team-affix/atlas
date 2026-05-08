#include "../../../hpp/domain/events/candidate_not_applicable_event.hpp"

std::ostream& operator<<(std::ostream& os, const candidate_not_applicable_event& e) {
    return os << "candidate_not_applicable_event{" << e.rl->idx << "}";
}
