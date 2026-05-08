#include "../../../hpp/application/event_handlers/inactive_goal_store_clearing_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

inactive_goal_store_clearing_event_handler::inactive_goal_store_clearing_event_handler() :
    inactive_goal_store(resolver::resolve<i_inactive_goal_store>()) {}

void inactive_goal_store_clearing_event_handler::handle(const goal_stores_clearing_event&) {
    inactive_goal_store.clear();
}
