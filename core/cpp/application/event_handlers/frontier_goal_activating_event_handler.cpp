#include "../../../hpp/application/event_handlers/frontier_goal_activating_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

frontier_goal_activating_event_handler::frontier_goal_activating_event_handler() :
    frontier_(locator::locate<i_frontier>()),
    goal_factory_(locator::locate<i_factory<goal, const goal_lineage*>>()) {}

void frontier_goal_activating_event_handler::handle(const goal_activating_event& event) {
    frontier_.insert(event.gl, goal_factory_.make(event.gl));
}
