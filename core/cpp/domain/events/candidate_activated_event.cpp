#include "../../../hpp/domain/events/candidate_activated_event.hpp"

std::ostream& operator<<(std::ostream& os, const candidate_activated_event& e) {
    return os << "candidate_activated_event{" << e.rl->idx << "}";
}
