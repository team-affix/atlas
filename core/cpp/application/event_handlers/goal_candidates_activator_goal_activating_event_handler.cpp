#include "../../../hpp/application/event_handlers/goal_candidates_activator_goal_activating_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

goal_candidates_activator_goal_activating_event_handler::goal_candidates_activator_goal_activating_event_handler() :
    goal_candidates_activator(locator::locate<i_goal_candidates_activator>()) {}

void goal_candidates_activator_goal_activating_event_handler::handle(const goal_activating_event& e) {
    goal_candidates_activator.init_activate(e.gl);
}
