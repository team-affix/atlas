#include "../../../hpp/application/event_handlers/goal_candidates_store_clearing_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

goal_candidates_store_clearing_event_handler::goal_candidates_store_clearing_event_handler() :
    goal_candidates_store(resolver::resolve<i_goal_candidates_store>()) {}

void goal_candidates_store_clearing_event_handler::handle(const goal_stores_clearing_event&) {
    goal_candidates_store.clear();
}
