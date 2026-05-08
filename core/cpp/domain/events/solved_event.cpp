#include "../../../hpp/domain/events/solved_event.hpp"

std::ostream& operator<<(std::ostream& os, const sim_solved_event&) {
    return os << "sim_solved_event{}";
}
