#include "../../../hpp/application/event_handlers/goal_expr_store_clearing_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

goal_expr_store_clearing_event_handler::goal_expr_store_clearing_event_handler() :
    goal_expr_store(resolver::resolve<i_goal_expr_store>()) {}

void goal_expr_store_clearing_event_handler::handle(const goal_stores_clearing_event&) {
    goal_expr_store.clear();
}
