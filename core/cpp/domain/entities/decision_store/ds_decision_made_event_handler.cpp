#include "../../../../hpp/domain/entities/decision_store/ds_decision_made_event_handler.hpp"
#include "../../../../hpp/infrastructure/locator.hpp"

ds_decision_made_event_handler::ds_decision_made_event_handler() :
    ds(locator::locate<decision_store>()) {
}

void ds_decision_made_event_handler::operator()(const decision_made_event& e) {
    ds.insert(e.rl);
}
