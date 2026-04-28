#include "../../../../hpp/domain/entities/cdcl/cdcl_sim_started_event_handler.hpp"
#include "../../../../hpp/infrastructure/locator.hpp"

cdcl_sim_started_event_handler::cdcl_sim_started_event_handler() :
    c(locator::locate<cdcl>()) {
}

void cdcl_sim_started_event_handler::operator()(const sim_started_event& e) {
    if (c.check_for_conflict())
        return;
    c.emit_eliminated_candidates();
}
