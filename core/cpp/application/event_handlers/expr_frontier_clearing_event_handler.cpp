#include "../../../hpp/application/event_handlers/expr_frontier_clearing_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

expr_frontier_clearing_event_handler::expr_frontier_clearing_event_handler() :
    ef(locator::locate<i_expr_frontier>()) {}

void expr_frontier_clearing_event_handler::handle(const goal_stores_clearing_event&) {
    ef.clear();
}
