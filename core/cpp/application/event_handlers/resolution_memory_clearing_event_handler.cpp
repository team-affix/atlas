#include "../../../hpp/application/event_handlers/resolution_memory_clearing_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

resolution_memory_clearing_event_handler::resolution_memory_clearing_event_handler() :
    resolution_memory(locator::locate<i_resolution_memory>()) {}

void resolution_memory_clearing_event_handler::handle(const goal_stores_clearing_event&) {
    resolution_memory.clear();
}
