#include "../../../../hpp/domain/entities/cdcl/cdcl_goal_resolved_event_handler.hpp"
#include "../../../../hpp/infrastructure/locator.hpp"

cdcl_goal_resolved_event_handler::cdcl_goal_resolved_event_handler() :
    c(locator::locate<cdcl>()) {
}

void cdcl_goal_resolved_event_handler::operator()(const goal_resolved_event& e) {
    c.constrain(e.rl);
}
