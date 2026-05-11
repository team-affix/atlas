#include "../../../hpp/application/event_handlers/weight_frontier_clearing_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

weight_frontier_clearing_event_handler::weight_frontier_clearing_event_handler() :
    wf(locator::resolve<i_weight_frontier>()) {}

void weight_frontier_clearing_event_handler::handle(const goal_stores_clearing_event&) {
    wf.clear();
}
