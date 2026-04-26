#include "../../../../hpp/domain/entities/candidate_store/cs_resolution_event_handler.hpp"
#include "../../../../hpp/infrastructure/locator.hpp"

cs_resolution_event_handler::cs_resolution_event_handler() :
    cs(locator::locate<candidate_store>()) {
}

void cs_resolution_event_handler::operator()(const resolution_event& e) {
    cs.resolve(e.rl);
}
