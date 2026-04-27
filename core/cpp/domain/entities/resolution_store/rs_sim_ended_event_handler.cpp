#include "../../../../hpp/domain/entities/resolution_store/rs_sim_ended_event_handler.hpp"
#include "../../../../hpp/infrastructure/locator.hpp"

rs_sim_ended_event_handler::rs_sim_ended_event_handler() :
    rs(locator::locate<resolution_store>()) {
}

void rs_sim_ended_event_handler::operator()(const sim_ended_event&) {
    rs.clear();
}
