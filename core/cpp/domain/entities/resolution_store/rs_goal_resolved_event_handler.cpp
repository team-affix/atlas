#include "../../../../hpp/domain/entities/resolution_store/rs_goal_resolved_event_handler.hpp"
#include "../../../../hpp/infrastructure/locator.hpp"

rs_goal_resolved_event_handler::rs_goal_resolved_event_handler() :
    rs(locator::locate<resolution_store>()) {
}

void rs_goal_resolved_event_handler::operator()(const goal_resolved_event& e) {
    rs.insert(e.rl);
}
