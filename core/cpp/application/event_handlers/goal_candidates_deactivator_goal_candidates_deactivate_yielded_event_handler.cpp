#include "../../../hpp/application/event_handlers/goal_candidates_deactivator_goal_candidates_deactivate_yielded_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

goal_candidates_deactivator_goal_candidates_deactivate_yielded_event_handler::goal_candidates_deactivator_goal_candidates_deactivate_yielded_event_handler() :
    goal_candidates_deactivator(locator::locate<i_goal_candidates_deactivator>()) {}

void goal_candidates_deactivator_goal_candidates_deactivate_yielded_event_handler::handle(const goal_candidates_deactivate_yielded_event&) {
    goal_candidates_deactivator.resume();
}
