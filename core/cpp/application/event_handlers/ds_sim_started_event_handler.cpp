#include "../../../../hpp/domain/entities/decision_store/ds_sim_started_event_handler.hpp"
#include "../../../../hpp/infrastructure/locator.hpp"

ds_sim_started_event_handler::ds_sim_started_event_handler() :
    ds(locator::locate<decision_store>()) {
}

void ds_sim_started_event_handler::operator()(const sim_started_event&) {
    ds.clear();
}
