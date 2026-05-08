#include "../../../hpp/domain/events/no_more_unit_goals_event.hpp"

std::ostream& operator<<(std::ostream& os, const no_more_unit_goals_event&) {
    return os << "no_more_unit_goals_event{}";
}
