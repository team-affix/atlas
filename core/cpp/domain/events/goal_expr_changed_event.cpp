#include "../../../hpp/domain/events/goal_expr_changed_event.hpp"

std::ostream& operator<<(std::ostream& os, const goal_expr_changed_event& e) {
    return os << "goal_expr_changed_event{" << e.gl->idx << "}";
}
