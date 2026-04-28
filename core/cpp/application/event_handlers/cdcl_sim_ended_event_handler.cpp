#include "../../../../hpp/domain/entities/cdcl/cdcl_sim_ended_event_handler.hpp"
#include "../../../../hpp/infrastructure/locator.hpp"

cdcl_sim_ended_event_handler::cdcl_sim_ended_event_handler() :
    ds(locator::locate<decision_store>()),
    c(locator::locate<cdcl>()) {
}

void cdcl_sim_ended_event_handler::operator()(const sim_ended_event& e) {
    c.learn(ds.get());
}
