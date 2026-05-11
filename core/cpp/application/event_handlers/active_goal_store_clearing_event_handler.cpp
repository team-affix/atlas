#include "../../../hpp/application/event_handlers/active_goal_store_clearing_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

active_goal_store_clearing_event_handler::active_goal_store_clearing_event_handler() :
    active_goal_store(locator::locate<i_active_goal_store>()) {}

void active_goal_store_clearing_event_handler::handle(const goal_stores_clearing_event&) {
    active_goal_store.clear();
}
