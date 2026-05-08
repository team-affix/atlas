#include "../../../hpp/application/event_handlers/goal_deactivated_active_goals_empty_detector_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

goal_deactivated_active_goals_empty_detector_event_handler::goal_deactivated_active_goals_empty_detector_event_handler() :
    active_goals_empty_detector(resolver::resolve<i_active_goals_empty_detector>()) {}

void goal_deactivated_active_goals_empty_detector_event_handler::handle(const goal_deactivated_event&) {
    active_goals_empty_detector.goal_deactivated();
}
