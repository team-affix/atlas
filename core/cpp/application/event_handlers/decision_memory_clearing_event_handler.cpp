#include "../../../hpp/application/event_handlers/decision_memory_clearing_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

decision_memory_clearing_event_handler::decision_memory_clearing_event_handler() :
    decision_memory(locator::resolve<i_decision_memory>()) {}

void decision_memory_clearing_event_handler::handle(const goal_stores_clearing_event&) {
    decision_memory.clear();
}
