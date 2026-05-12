#include "../../../hpp/domain/entities/active_eliminator.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

active_eliminator::active_eliminator()
    : gcs(locator::locate<i_candidates_frontier>()),
    candidate_eliminating_producer(locator::locate<i_event_producer<candidate_eliminating_event>>()),
    candidate_eliminated_producer(locator::locate<i_event_producer<candidate_eliminated_event>>()),
    eliminate_candidate_yield_producer(locator::locate<i_event_producer<eliminate_candidate_yield_event>>()) {}

void active_eliminator::eliminate(const resolution_lineage* rl) {
    current_rl = rl;
    gcs.at(rl->parent).candidates.erase(rl->idx);
    candidate_eliminating_producer.produce({rl});
    eliminate_candidate_yield_producer.produce({});
}

void active_eliminator::resume() {
    candidate_eliminated_producer.produce({current_rl});
}
