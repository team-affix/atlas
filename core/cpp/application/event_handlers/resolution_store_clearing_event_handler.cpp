#include "../../../hpp/application/event_handlers/resolution_store_clearing_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

resolution_store_clearing_event_handler::resolution_store_clearing_event_handler() :
    resolution_store(resolver::resolve<i_resolution_store>()) {}

void resolution_store_clearing_event_handler::handle(const goal_stores_clearing_event&) {
    resolution_store.clear();
}
