#include "../../../hpp/application/event_handlers/expr_frontier_clearing_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

expr_frontier_clearing_event_handler::expr_frontier_clearing_event_handler() :
    ef(resolver::resolve<i_expr_frontier>()) {}

void expr_frontier_clearing_event_handler::handle(const goal_stores_clearing_event&) {
    ef.clear();
}
