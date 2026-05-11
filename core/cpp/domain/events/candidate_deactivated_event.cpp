#include "../../../hpp/domain/events/candidate_deactivated_event.hpp"

std::ostream& operator<<(std::ostream& os, const candidate_deactivated_event& e) {
    return os << "candidate_deactivated_event{" << e.rl->idx << "}";
}
