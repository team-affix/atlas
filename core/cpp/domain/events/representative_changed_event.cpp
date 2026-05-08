#include "../../../hpp/domain/events/representative_changed_event.hpp"

std::ostream& operator<<(std::ostream& os, const representative_changed_event& e) {
    return os << "representative_changed_event{" << e.var_index << "}";
}
