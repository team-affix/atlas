#include "../../../hpp/application/event_handlers/candidates_frontier_clearing_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

candidates_frontier_clearing_event_handler::candidates_frontier_clearing_event_handler() :
    cf(locator::locate<i_candidates_frontier>()) {}

void candidates_frontier_clearing_event_handler::handle(const goal_stores_clearing_event&) {
    cf.clear();
}
