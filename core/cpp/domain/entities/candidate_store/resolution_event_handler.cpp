#include "../../../../hpp/domain/entities/candidate_store/resolution_event_handler.hpp"
#include "../../../../hpp/infrastructure/locator.hpp"

resolution_event_handler::resolution_event_handler() :
    cs(locator::locate<candidate_store>()) {
}

void resolution_event_handler::operator()(const resolution_event& e) {
    cs.resolve(e.rl);
}
