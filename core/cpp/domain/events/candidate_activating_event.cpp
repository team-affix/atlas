#include "../../../hpp/domain/events/candidate_activating_event.hpp"

std::ostream& operator<<(std::ostream& os, const candidate_activating_event& e) {
    return os << "candidate_activating_event{" << e.rl->idx << "}";
}
