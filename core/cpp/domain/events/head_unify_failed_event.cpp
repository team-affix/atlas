#include "../../../hpp/domain/events/head_unify_failed_event.hpp"

std::ostream& operator<<(std::ostream& os, const head_unify_failed_event& e) {
    return os << "head_unify_failed_event{" << e.rl->idx << "}";
}
