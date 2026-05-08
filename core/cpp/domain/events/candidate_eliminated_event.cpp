#include "../../../hpp/domain/events/candidate_eliminated_event.hpp"

std::ostream& operator<<(std::ostream& os, const candidate_eliminated_event& e) {
    return os << "candidate_eliminated_event{" << e.rl->idx << "}";
}
