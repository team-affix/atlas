#include "../../../hpp/application/event_handlers/goal_stores_cleared_sim_stopper_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

goal_stores_cleared_sim_stopper_event_handler::goal_stores_cleared_sim_stopper_event_handler() :
    sim_stopper(locator::locate<i_sim_stopper>()) {}

void goal_stores_cleared_sim_stopper_event_handler::handle(const goal_stores_cleared_event&) {
    sim_stopper.finish_stop();
}
