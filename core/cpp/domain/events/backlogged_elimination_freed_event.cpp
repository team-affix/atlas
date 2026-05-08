#include "../../../hpp/domain/events/backlogged_elimination_freed_event.hpp"

std::ostream& operator<<(std::ostream& os, const backlogged_elimination_freed_event& e) {
    return os << "backlogged_elimination_freed_event{" << e.gl->idx << ", " << e.idx << "}";
}
