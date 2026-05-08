#include "../../../hpp/application/event_handlers/decision_store_deciding_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

decision_store_deciding_event_handler::decision_store_deciding_event_handler() :
    decision_store(resolver::resolve<i_decision_store>()) {}

void decision_store_deciding_event_handler::handle(const deciding_event& e) {
    decision_store.insert(e.rl);
}
