#include "../../../hpp/application/event_handlers/goal_candidates_deactivator_goal_deactivating_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

goal_candidates_deactivator_goal_deactivating_event_handler::goal_candidates_deactivator_goal_deactivating_event_handler() :
    goal_candidates_deactivator(locator::locate<i_goal_candidates_deactivator>()) {}

void goal_candidates_deactivator_goal_deactivating_event_handler::handle(const goal_deactivating_event& e) {
    goal_candidates_deactivator.init_deactivate(e.gl);
}
