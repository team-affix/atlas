#include "../../../hpp/application/event_handlers/decision_store_clearing_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

decision_store_clearing_event_handler::decision_store_clearing_event_handler() :
    decision_store(resolver::resolve<i_decision_store>()) {}

void decision_store_clearing_event_handler::handle(const goal_stores_clearing_event&) {
    decision_store.clear();
}
