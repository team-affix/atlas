#include "../../../../hpp/domain/entities/candidate_store/eliminate_candidate_event_handler.hpp"
#include "../../../../hpp/infrastructure/locator.hpp"

eliminate_candidate_event_handler::eliminate_candidate_event_handler() :
    cs(locator::locate<candidate_store>()) {
}

void eliminate_candidate_event_handler::operator()(const eliminate_candidate_event& e) {
    auto gl = e.rl->parent;
    auto idx = e.rl->idx;
    cs.at(gl).erase(idx);
}
