#include "../../../hpp/application/event_handlers/goal_weight_store_clearing_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

goal_weight_store_clearing_event_handler::goal_weight_store_clearing_event_handler() :
    goal_weight_store(resolver::resolve<i_goal_weight_store>()) {}

void goal_weight_store_clearing_event_handler::handle(const goal_stores_clearing_event&) {
    goal_weight_store.clear();
}
