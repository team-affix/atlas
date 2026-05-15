#include "../../../hpp/application/event_handlers/frontier_goal_deactivating_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

frontier_goal_deactivating_event_handler::frontier_goal_deactivating_event_handler() :
    frontier_(locator::locate<i_frontier>()) {}

void frontier_goal_deactivating_event_handler::handle(const goal_deactivating_event& event) {
    frontier_.erase(event.gl);
}
